# OrcStream Wire Draft 0.1 (experimental, changeable)

**Status:** implementation experiment, not a published interoperable standard. Frame layout and semantics may change. No VITA-49 conformance claim. Source-grounded priorities come from the SDR++ RSA/REA research, but no upstream SDR++ code is copied here.

**MVP boundary:** IQ data plane, one-way sender HELLO announcement, bounded parser, UDP and file examples. No listening/discovery, subscriptions, remote control, authentication, encryption, reliable delivery, device timestamp synchronization, compression, or multi-subscriber server. Those are future features, not implied by header fields.

## Frame encoding

Each frame is a fixed 64-byte header followed by exactly `payload_length` bytes. Wire integers are **big-endian unsigned**. Raw IQ bytes are in the explicitly declared I/Q sample format (which may be little-endian). Header has no alignment or padding; host C structs are never copied into the wire. A byte stream may concatenate frames directly, while one datagram should carry **exactly one frame**. The current maximum payload is 65536 bytes; UDP implementations should use a substantially smaller payload (e.g. 1152 bytes) to avoid IP fragmentation.

| Offset | Size | Field | Meaning |
| ---: | ---: | --- | --- |
| 0 | 4 | magic | ASCII `ORCS` |
| 4 | 1 | major | 0 |
| 5 | 1 | minor | 1 |
| 6 | 2 | header_size | 64 |
| 8 | 1 | packet_type | 1 = HELLO, 2 = IQ |
| 9 | 1 | sample_format | 0 for HELLO, 1..6 for IQ |
| 10 | 2 | flags | See below |
| 12 | 4 | stream_id | Nonzero for IQ; zero for HELLO |
| 16 | 8 | sequence | IQ packet sequence within stream, per-stream |
| 24 | 8 | sample_index | Complex-sample index of first I/Q pair |
| 32 | 8 | timestamp_ns | First sample time, valid only when flagged |
| 40 | 8 | center_frequency_hz | RF center in integer Hz, valid only when flagged |
| 48 | 4 | sample_rate_num | Numerator of complex samples/second |
| 52 | 4 | sample_rate_den | Denominator, nonzero for IQ |
| 56 | 4 | payload_length | Payload bytes, 0..65536 |
| 60 | 4 | crc32 | CRC of header bytes 0..59 followed by payload |
| 64 | N | payload | Format-specific samples or HELLO |

CRC uses IEEE CRC-32 (reflected polynomial `0xEDB88320`, initialization `0xFFFFFFFF`, final XOR `0xFFFFFFFF`), transmitted as a big-endian 32-bit integer. CRC is only accidental corruption detection, **not** authentication, encryption, anti-replay or an integrity guarantee against an attacker. An invalid frame is rejected and the caller decides whether to reconnect or discard; the byte-stream reader does not scan for magic to resynchronize.

### Packet types

- `HELLO` (type 1): exactly 16 bytes, format 0 and all other metadata fields zero. It advertises supported formats and a proposed capacity. It is **not a negotiated session** in this draft. The examples send it first; API parsing does not require it to appear first.
- `IQ` (type 2): nonzero stream ID, sample-rate numerator and denominator. Payload is zero length only when END_OF_STREAM is set. Payload size must be a multiple of the complex sample size. Sample index must not overflow 64-bit when adding payload sample count.

### HELLO payload (16 bytes, big-endian)

| Offset | Size | Name |
| ---: | ---: | --- |
| 0 | 4 | capabilities: bit 0 = IQ RX only |
| 4 | 4 | format_mask: bit N = format N supported |
| 8 | 4 | max_payload (1..65536) |
| 12 | 4 | max_streams (positive) |

Unsupported capability bits are rejected. For now `HELLO` has no control-plane or security negotiation semantics and must not be used to claim such capability.

### IQ sample formats

| ID | Format | Bytes per complex sample | Values / byte order |
| ---: | --- | ---: | --- |
| 1 | CU8 | 2 | unsigned I, unsigned Q; nominal midpoint 128 |
| 2 | CS8 | 2 | signed two's-complement I, Q |
| 3 | CS16_LE | 4 | signed 16-bit I, Q, little-endian |
| 4 | CS16_BE | 4 | signed 16-bit I, Q, big-endian |
| 5 | CF32_LE | 8 | I, Q as IEEE 754 binary32, little-endian |
| 6 | CF32_BE | 8 | I, Q as IEEE 754 binary32, big-endian |

**No conversion is performed** by this SDK. Encoder and decoder transport the exact supplied payload bytes. Sample scaling, resampling, quantization or an RF frontend's effective numeric range are responsibilities of explicit adapters and must be measured, not assumed. Signed CU8 versus CS8 must never be implicitly mixed.

### Flags (bits not listed are invalid)

| Bit mask | Name | Semantics |
| --- | --- | --- |
| 0x0001 | TIMESTAMP_VALID | `timestamp_ns` describes first sample |
| 0x0002 | DISCONTINUITY | Source declares stream sample epoch/reset or discontinuity |
| 0x0004 | END_OF_STREAM | End marker; may include final IQ payload or zero samples |
| 0x0008 | FREQUENCY_VALID | `center_frequency_hz` is known; zero without this flag |
| 0x0010 | CLOCK_MONOTONIC | Timestamp is local monotonic nanoseconds; NOT comparable across devices |
| 0x0020 | CLOCK_UTC | Timestamp is Unix UTC nanoseconds, if device has a trustworthy clock |
| 0x0040 | CLOCK_DEVICE | Timestamp is nanoseconds on device clock domain, origin unspecified |

Timestamp VALID requires **exactly one** clock domain. If not VALID, both clock domain bits and timestamp value must be zero. Values are not synthesized if the source hardware cannot timestamp. In particular, clock domain DEVICE does not indicate GPS discipline or a globally synchronized epoch. Frequency zero may be a real value when FREQUENCY_VALID is set; without it zero means unknown.

### Continuity / sequence

Every IQ frame increments the per-stream `sequence` by one and advances `sample_index` by the number of complex samples in its payload. A new stream ID or explicit DISCONTINUITY is needed when a source loses continuity or resets numbering. The tracker can count sequence gaps, sample-index gaps, and late-or-duplicate packets; it cannot distinguish a true late arrival from a duplicate without additional state. **A CRC-passing frame is not a proof that all prior samples arrived.** Sequence state is external for separate stream IDs. Packet sequence exhaustion at 2^64-1 requires starting a fresh stream ID before wrap; a stable production epoch scheme remains to be specified.

## Example byte layout

A HELLO frame is 80 bytes: `ORCS` + version `(00 01)` + header size `(00 40)` + type `(01)` + format `(00)` + all-zero metadata, followed by a 16-byte HELLO payload; header bytes 60..63 contain a CRC over bytes 0..59 concatenated with the 16 payload bytes. Tests enforce field offsets and validate CRC in both directions.

## Protocol layering

```
        [OrcSDR / other capture device / file replay]
                       |
             [OrcStream C99 packet core]
                       |
               [Transport adapter]
           file | UDP | TCP | future link
                       |
             [OrcStream packet consumer]
                       |
          [app / browser bridge / decoder]
```

The packet core does **not** depend on TCP, POSIX, ESP-IDF, Windows, allocations, threads, libusb, or a particular SDR tuner. The UDP example is separate. In constrained links, radio bandwidth/MTU limitations may make full-rate IQ physically impossible. 2.4 MS/s CU8 is 4,800,000 payload bytes/s (38.4 Mbit/s) **before frame and link overhead**.

## Security and deployment

This draft has no authentication, encryption or access control. It is not suitable for exposure on public networks. The sample UDP receiver intentionally binds only `127.0.0.1`. Future remote device control must use an authenticated and authorized control plane, separate from IQ data transport. CRC32 does not change that requirement.

## Compatibility scope

This draft is **not** VITA-49, rtl_tcp, SpyServer, SDR++ Server or SoapyRemote on the wire. Supporting them requires explicit separately licensed adapters, with their own conformance tests. No published benchmark comparing those implementations has been run as part of this milestone.

## Versioning rule for v0.1

Reject unknown major/minor or header size, flags, packet types and formats; this prevents silent misinterpretation. Later experimental versions may add an extension mechanism before a v1.0 freeze. No backward compatibility is guaranteed for v0.x.
