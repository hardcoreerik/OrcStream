# OrcStream v1 Wire Protocol — concrete **discussion draft**, NOT a released v1

**Design snapshot:** 2026-11-21 · **Protocol version proposed:** `1.0-draft-agent3` · **Normative status:** **NONE** until engineering review, two independent decoder implementations, conformance vectors and explicit project authorization. All values labeled `PROPOSED`; stable IANA/vendor registration and real encryption/auth implementation are not supplied by this document.

**Breaking notice:** The existing experimental `v0.1` 64-byte header uses different offsets and packet types. This proposal uses a **new 80-byte fixed header and major=1** with a codec registry, decoded length, context revision and epoch. **Do not** transmit this as v0.1, reinterpret v0.1 reserved flags, or claim wire compatibility. Reference: https://github.com/hardcoreerik/OrcStream/blob/27c134060631a2efb74da04ee760e49957474d96/docs/wire-v0.1.md

## 1. Framing

Every frame comprises exactly one 80-byte header, optional TLV extensions (currently none for v1 base), and `encoded_bytes` payload bytes. Fixed header integers are **big-endian unsigned**; sample payload endianness is declared by `source_format`. A byte stream may concatenate frames; UDP profile uses **one complete frame per datagram**, no IP fragmentation. Frame contains no copied host C structs or platform pointer sizes.

**Proposed wire header (80 bytes, offsets zero-based):**

| Offset | Size | Name | Rule / value |
|---:|---:|---|---|
| 0 | 4 | `magic` | ASCII `ORCS` (`4F 52 43 53`) |
| 4 | 1 | `major` | 1; reject unknown major |
| 5 | 1 | `minor` | 0; see extension compatibility rule |
| 6 | 2 | `header_bytes` | 80 for baseline, extensible only after TLV/header rule approved |
| 8 | 1 | `packet_type` | numbered below |
| 9 | 1 | `flags` | bit mask below; unknown set bits reject for major 1 |
| 10 | 2 | `codec_id` | `0` raw identity; `1` exact CS12 pack; others negotiated |
| 12 | 4 | `stream_id` | 0 for unbound handshake, nonzero for subscribed data |
| 16 | 4 | `epoch` | stream generation, nonzero for IQ frames |
| 20 | 8 | `sequence` | per-stream-epoch IQ frame ordinal, starting at 0 |
| 28 | 8 | `sample_index` | first **complex** sample index in stream epoch |
| 36 | 4 | `sample_count` | complex samples in decoded IQ frame |
| 40 | 2 | `source_format` | declared *decoded* source sample representation |
| 42 | 2 | `codec_param_id` | refers to immutable negotiated codec profile; 0 for baseline raw/CS12 packing |
| 44 | 4 | `rate_num` | rational complex samples/s numerator; nonzero for IQ |
| 48 | 4 | `rate_den` | denominator; nonzero for IQ |
| 52 | 8 | `timestamp_ns` | first-sample time, valid only with flag and known clock domain |
| 60 | 4 | `clock_id` | 0 if no timestamp; otherwise context-defined domain ID |
| 64 | 4 | `encoded_bytes` | payload bytes as transmitted, max per negotiated session |
| 68 | 4 | `decoded_bytes` | exact post-decode bytes for packet's declared output format |
| 72 | 4 | `crc32` | IEEE CRC-32 as described below |
| 76 | 4 | `context_revision` | bound metadata revision; nonzero for IQ frames |

**CRC (proposed):** reflected CRC-32/IEEE polynomial `0xEDB88320`, init `0xFFFFFFFF`, final XOR `0xFFFFFFFF`, over bytes `header[0:72] || header[76:80] || payload` (and in future any approved extension bytes between fixed header and payload, in their wire order). Result stored BE at offset 72. CRC is noncryptographic corruption detection and cannot authenticate a peer. Any change to this scope requires a new formal major-version profile.

**Frame size:** baseline parser must reject >65,536 encoded payload bytes and >65,536 declared decoded bytes **before** allocation; tighter per-peer negotiated limits apply (e.g. 4 KiB output on Tier 0). Packet size multiplication `sample_count × bytes_per_complex` must use checked 64-bit intermediates before comparing decoded size. For smaller-than-full MTU UDP, agreed complete frame <=1152 bytes unless path probing specifies a safe greater value; thus with 80-byte header at most 1072 payload bytes. No guarantee this survives all VPNs/tunnels; count network encapsulation explicitly.

## 2. Packet types and flags

| Type | ID proposed | Transport/direction and purpose |
|---|---:|---|
| `CLIENT_HELLO` | 1 | client→server; versions, formats, decoder resource budget and authentication context |
| `CAPABILITIES` | 2 | server→client; authoritative supported source formats/codecs, metadata and live limits |
| `SUBSCRIBE` | 3 | client→server; stream family, selected source/channel, fidelity and permitted fallbacks |
| `ACCEPT` | 4 | server→client; new stream ID/epoch, codec profile, limits |
| `IQ` | 5 | producer→subscriber; exact or explicitly lossy IQ, framed as above |
| `CONTEXT` | 6 | server→client; clock, LO, source sample format, channel, transform and codec param mapping |
| `REPORT` | 7 | either; stats, known/unknown gaps, resources and heartbeat |
| `ERROR` | 8 | either; protocol/capability/security/bandwidth/codec error |
| `GOODBYE` | 9 | either; explicit session/stream shutdown |
| `PRODUCT` | 10 | producer→subscriber; explicitly derived channel/spectrum/event/trigger product, non-IQ family descriptor mandatory |

`flags` bit0 `TIMESTAMP_VALID`; bit1 `DISCONTINUITY` (must identify new epoch or explicit documented gap); bit2 `END_OF_STREAM` (may be zero samples with a new sequenced IQ header); bits3–7 **reserved, reject if nonzero**. `CLOCK_ID` is **not** a bit flag; source contexts describe it. If timestamp flag unset, `timestamp_ns=0` and `clock_id=0` are mandatory. First valid UTC timestamp only with verified synchronization and uncertainty context; otherwise use device/monotonic domain ID or none.

For non-IQ packets, `codec_id=0`, `source_format=0`, `codec_param_id=0`, `sample_index=0`, `sample_count=0`, `rate_num=0`, `rate_den=0`, `decoded_bytes=encoded_bytes` and `sequence=0` unless a later normative packet-family specification changes those conventions. `stream_id=0` until a stream is accepted; stream-targeted `CONTEXT/REPORT/ERROR` may carry nonzero stream ID and epoch. A parser must still inspect non-IQ payload type and TLV lengths.

## 3. Source sample formats and mandatory codecs

| Format ID (proposed) | Decoded source words, I first then Q | Bytes per complex |
|---|---|---:|
| 1 | CU8 (unsigned midpoint 128) | 2 |
| 2 | CS8 two's complement | 2 |
| 3 | CS16_LE, interleaved I/Q | 4 |
| 4 | CS16_BE, interleaved I/Q | 4 |
| 5 | CF32_LE, IEEE-754 bitwise representation | 8 |
| 6 | CF32_BE, IEEE-754 bitwise representation | 8 |

**Mandatory for every v1 core SDK:** full parsing of all IDs and **identity (`codec_id=0`)** for supported format(s), with exact byte preservation. Implementations must advertise what they actually ingest/emit; a CU8-only producer must not fake native CS16 precision. Recommended minimum receiver implementation covers CU8 and CS16_LE; don't impose conversion on an 8-bit source. This receiver-format recommendation remains **UNDECIDED** before version freeze.

**`codec_id=1` = `PACK_SIGNED12_IQ_V1` (optional, negotiated, Mode A):** input `source_format=CS16_LE` or `CS16_BE` with `valid_bits=12` and canonical 12-bit signed two's complement verified at the source; output: 3 bytes per complex sample, packing `byte0=I[7:0]`, `byte1=(I[11:8]) | (Q[3:0] << 4)`, `byte2=Q[11:4]`. Decode **sign-extend** each 12-bit field and return the source format's 16-bit words; `decoded_bytes=sample_count×4`, `encoded_bytes=sample_count×3`; no other payload padding. If source samples exceed [-2048,2047], **reject** the packing operation or use a separately negotiated raw block; never truncate without explicit fidelity change.

**`codec_id=0` = `IDENTITY_V1` (mandatory):** payload is byte-for-byte the output source sample representation, and `decoded_bytes=encoded_bytes=sample_count×bytes_per_complex` for IQ frames. The SDK **must not silently convert CF32⇄CS16 or CU8⇄CS8** in identity. If no native capture path can meet transport bandwidth, reject admission or report explicit drops without inventing lossless capacity.

**Codec IDs 2+ are `UNASSIGNED` pending an open registry.** Recommended future modules: Rice predictor, zstd/LZ4, fixed-point BFP8/10 and quantizers, but no numeric identifiers should be frozen without exact bit vectors and resource profiles. Unknown codec ID always errors unless explicitly negotiated and supported; `codec_param_id` is a session-scoped immutable definition and can change only with new context revision or epoch.

## 4. Control payload: bounded TLVs

Control and context payloads consist of repeated `uint16_be type`, `uint16_be value_length`, then exactly `value_length` bytes. No implicit alignment/padding or nested indefinite lengths. TLV type bit15 set means **critical**: unknown critical TLV MUST reject that message; unknown noncritical TLV MAY be skipped after exact length validation. No duplicate critical singleton keys. Contexts have a monotonically incrementing revision *per stream epoch*. Max aggregate TLV payload bounded by negotiated max. Text fields use length-prefixed UTF-8 with no requirement to parse Unicode on Tier 0; security-critical identifiers use fixed-width binary encoding.

**Suggested TLV registry (proposed, details require review):**

| ID | TLV | Proposed value |
|---:|---|---|
| 0x8001 | `VERSIONS` | ordered pairs of `(major:u8,minor:u8)`; must contain version 1.0 for draft peer |
| 0x8002 | `MAX_FRAME` | `max_encoded:u32, max_decoded:u32, max_scratch:u32` |
| 0x8003 | `FORMAT_LIST` | count u16 + repeated `source_format:u16,valid_bits:u8,flags:u8` |
| 0x8004 | `CODEC_LIST` | count u16 + tuples `codec_id:u16,revision:u16` |
| 0x8005 | `FIDELITY_POLICY` | mode:u8 + quantization/error profile identifier u16 + fallback mask:u8; detailed limits in extension |
| 0x8006 | `STREAM_IDENTITY` | source UUID 16 bytes, logical channel UUID 16 bytes, stream family:u16 |
| 0x8007 | `SOURCE_CONTEXT` | frequency Hz u64, actual rate rational u32/u32, channel order, signed valid bits, epoch and LO group (exact sublayout **UNDECIDED**) |
| 0x8008 | `CLOCK_CONTEXT` | id:u32, domain:u8, sync validity:u8, uncertainty_ns:u64, origin fields (exact layout **UNDECIDED**) |
| 0x8009 | `CODEC_PROFILE` | id:u16, codec revision:u16, bounded block/input/output sizes, parameter bytes (precise codec-specific schemas **UNDECIDED**) |
| 0x800A | `GAP_REPORT` | stage:u8, known_count:u8, first_index:u64, lost_count:u64 (count ignored when unknown); schema **UNDECIDED** for multi-stage telemetry |
| 0x800B | `SUBSCRIBE_REQUEST` | source/channel refs, stream family, requested format and fidelity, max wire rate and fallback mask (schema **UNDECIDED**) |
| 0x800C | `STATUS_ERROR` | error code:u16 + bounded payload detail |
| 0x000D | `BUILD_ID` | optional immutable firmware, PL, OS and SDK build hashes |

A draft cannot be called fully interoperable while mandatory TLV layouts `SOURCE_CONTEXT`, `CLOCK_CONTEXT` and `SUBSCRIBE_REQUEST` remain undecided. **Before implementing production version 1, freeze them and add vectors.** A Tier 0 decode-only file or one-way stream may have a bounded manifest instead of an active network subscription, but must convey equivalent context.

## 5. Session, replay and recovery

```text
START -> HELLO received -> authenticated/authorized transport accepted
     -> CAPABILITIES -> SUBSCRIBE -> ACCEPT -> CONTEXT acknowledged
     -> IQ/PRODUCT packets within declared policy
     -> REPORT/ERROR and optional renewed context/epoch
     -> GOODBYE / disconnect
```

- Remote config is **read-only by default**. For any remote tune, gain, GPIO, firmware change or future TX operation, require an authenticated/authorized control channel with replay protection and per-action quotas. TLS, QUIC TLS or a separately authenticated secure transport is an adapter decision; do not implement ad-hoc crypto in tiny nodes. **No cryptographic algorithm is mandatory for one-way wire parsing.** Public-network listening without authenticated control is unsupported.
- A frame CRC is not a MAC. Out-of-order UDP frames are expected; acceptance and optional reorder window based on negotiated resource budget. `sequence` resets to 0 on new epoch, and `sample_index` is complex-sample position in that epoch. Decoder must not turn a network sequence gap into a fabricated source sample gap without proof. Missing `CONTEXT` or stale `context_revision` means **cannot decode** until context is obtained or stream dropped explicitly.
- For stream packet loss, exact gap fields, known count vs unknown, and source/producer/network/subscriber cause must be distinguished. Per-client backpressure cannot block an ADC capture DMA ring. A retune, format/clock change or codec parameter reinterpretation starts new epoch; replay recordings retain original context snapshots.
- Byte-stream framing after corruption: in v1 draft, reject frame and close/reset until a separately specified resynchronization policy is negotiated. **Do not** scan for magic in arbitrary compressed payload and presume resynchronization. Fuzz and negative tests mandatory.

## 6. Codec/fidelity negotiation pseudocode

```text
allowed = intersection(client.decode_caps, source.encode_caps)
require source_native_format in request.formats
require requested fidelity class agreed (A exact by default)
require required_clock_quality if any
for each source block:
    try only codecs in allowed with matching fidelity and bounded work
    choose min(total_encoded_wire_bytes) subject to encode_deadline,
           scratch_limit, receiver_max_decoded_bytes
    if no candidate improves enough: use IDENTITY when allowed
    if bit-exact and insufficient wire capacity: FAIL / REPORT loss;
          do not silently switch to lower precision or derived products
    if profile is lossy: include exact quantization semantics,
          enforce numerical bounds, increment context/epoch if mode changes
```

Compression mode can change **per frame** only among negotiated codecs in the same fidelity semantics with fully independent frame decode/reset. A stateful codec's dictionary scope and checkpoint must be unambiguous. Derived products use the `PRODUCT` type and a separately versioned transform family; they are **not** `IQ` frames masquerading as raw.

## 7. Integrity, vector and parser acceptance

A sample executable draft vector generator is included in `../experiments/v1_draft_wire_vectors.py`. It builds one 3-complex-sample raw CS16_LE frame and its exact packed12 representation, computes CRC32 as above and emits JSON known-answer fixtures. A future Rust/Go/C99 independent parser must decode those without copying the generator. This is **proposal-level** conformance, not a stable SDK.

Required adversarial tests: magic/version/header/codec invalid; endian confusion; `source_format=CS16` with invalid valid-bits; `sample_count×stride` overflow; truncated payload; payload oversize; CRC corruption; duplicate critical TLVs; unknown critical TLV; invalid clock flags; stale context; malicious high decompression ratio; packet reorder/duplicate/loss; 64-bit index wrap; invalid sign extension; all-ones/zero CF32 bit patterns; metadata mismatch between epoch and decoded payload.

## 8. Decisions explicitly still open before v1.0

1. Precise TLV `SOURCE_CONTEXT`/`CLOCK_CONTEXT`/`SUBSCRIBE_REQUEST` structures, and how many of them are mandatory for offline one-way replay.
2. Whether fixed 80-byte per-packet header is worth overhead at very high MSPS and small UDP MTU; potential compact-profile header extension and adapter semantics.
3. Whether `rate_num/rate_den` belong on **every** packet or only in immutable context and an index-to-time mapping.
4. Whether a mandatory receiver must support both CU8 and CS16_LE or simply advertised identity formats.
5. Full codec ID version registry, algorithmic quantization and BFP rounding reference semantics and formal error policies.
6. Packet-level `PRODUCT` representation and codec policy; DIFI/VITA adapter mapping and binary-wire profile tests.
7. Source clock-domain uncertainty representation, sample counter overflow, epoch transition and unknown-duration loss.
8. Proof of transport mutual authentication and control authorization that remains deployable on minimal hardware.
9. Security and resource constraints for unknown future codecs and untrusted decompressors.
10. License/IPR review, public registry stewardship, cross-architecture ABI and signed release provenance.

**Do not freeze version 1 or advertise interoperability until these decisions are resolved.** Current independent source truth: https://github.com/hardcoreerik/OrcStream/blob/66c3a294d36c2ad1b38243b9146952cabb5aa22f/PROJECT_TRUTH.md
