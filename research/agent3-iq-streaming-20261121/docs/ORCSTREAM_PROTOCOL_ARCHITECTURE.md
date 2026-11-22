# OrcStream Protocol Architecture — hardware-independent research proposal

**Research / not implemented.** Snapshot 2026-11-21. **Primary truth:** [`PROJECT_TRUTH.md`](https://github.com/hardcoreerik/OrcStream/blob/66c3a294d36c2ad1b38243b9146952cabb5aa22f/PROJECT_TRUTH.md). **Experimental starting point:** [`wire-v0.1.md`](https://github.com/hardcoreerik/OrcStream/blob/27c134060631a2efb74da04ee760e49957474d96/docs/wire-v0.1.md). No implicit v0.1→v1 binary compatibility.

## Design thesis

**OrcStream is a source-agnostic representation, transport and control boundary.** No core `#include` may depend on AD9361, Xilinx, ESP-IDF, a Linux socket, a particular SDR application or a proprietary hardware firmware. A Pluto-compatible Zynq-7020/AD9361 board is a **reference producer**, and OrcSDR is one **reference consumer**, not requirements.

A portable C99 parser and framing/codec SDK is the smallest common denominator. A hardware driver owns actual source samples/timing/scan layout. A transport adapter owns delivery. Application policy owns choices about channelization, truncation, lossy fidelity, and whether to trust classification. A server owns client quotas and security.

```text
ADC / file / network source (unmodified physical samples)
   -> source adapter: formats, native counters, clock, proof of provenance
   -> optional source processing: bit packing / compression / FPGA DDC/FFT/events
   -> stream epoch + context + sample continuity manager
   -> OrcStream C99 data and control serializer + negotiated codec registry
   -> bounded transport: USB bulk/IP, TCP, UDP, QUIC bridge, serial, files
   -> bounded receiver parser + codec dispatch + gap reports
   -> generic sample API / SoapySDR / GNU Radio / OrcSDR / DIFI/VITA adapter
```

**Data plane** is independently decodable bounded IQ/product frames. **Control plane** is reliable version/capability/subscription/configuration and authorization; it may use an authenticated transport. **Observation plane** is telemetry, gap/error, provenance and metric metadata. There is no “magical compression” semantics in transport layer.

## Component contracts / ownership

| Module | Responsibilities | Forbidden coupling |
|---|---|---|
| `core/wire` | byte-order-safe encode/decode; lengths; versions; codec registry; semantic validation | raw sockets/USB, device names, floating-point assumptions |
| `core/iq` | native sample descriptions, bit-exact packing, tagged bounded-error codec, C99 scratch alloc caller-owned | sample-rate knobs, LO drivers |
| `core/context` | stream ID, channel ID, epoch, clock domain, source revisions, transform provenance | claiming GPS/UTC from free-running counter |
| `core/continuity` | source counter gaps, packet sequence gaps, duplicate/late detection, declared unknown loss | filling missing samples with fabricated data |
| `adapters/source` | RTL CU8, libiio CS16, file; source-native capabilities | assuming a universal AD9361 layout |
| `adapters/transport` | USB IP/bulk, TCP, UDP, QUIC bridges, serial, local file | implicit capture state and codec semantics |
| `services/session` | authenticate, negotiate, subscribe, reserve, revoke; resource limits | remote unrestricted GPIO/firmware writes |
| `services/processing` | optional channelizer, FFT, trigger/capture, inference | putting detector accuracy claims in IQ payload |
| `adapters/interop` | VRT/DIFI, Soapy, GNU Radio, rtl_tcp, SDR++ or SDRangel bridging | declaring binary compatibility without conformance |

## Negotiation state machine (proposed)

```text
DISCONNECTED
  -> CONNECT (authenticated transport or trusted local physical link)
  -> CLIENT_HELLO: [wire majors/minors, max frame bytes, format IDs,
                    codec IDs, fidelity classes, max decode scratch,
                    clock/time requirements, transport constraints]
  <- SERVER_CAPABILITIES: authoritative actual firmware/source/stream scopes
  -> SUBSCRIBE(stream family, exact source, channels, source format/fidelity,
               codec preference list, max output rate, latency budget,
               explicit fallback and error policy)
  <- ACCEPT(stream_id, epoch, context_revision, negotiated codec/fidelity,
            max frame sizes, allocation requirement, authoritative capability)
  -> START/READY
  <- IQ/PRODUCT frames + CONTEXT/TELEMETRY
  -> UNSUBSCRIBE / RECONFIGURE
  <- EPOCH_CHANGE or RELEASE / REPORT/ERROR
```

Only codecs/versions supported by **both endpoints** can be selected. An advanced server cannot send a complex decoder to an MCU without negotiation. `RAW_IQ` means source-native representation and no source-side information loss beyond what hardware already did; a receiver may optionally *request* a reversible packing representation. A lossy bitwidth or DDC path must be explicitly requested and marked; absent consent, sender reports link-capacity failure or drops with true gap metadata, never silently alters fidelity.

### Minimum negotiable capability descriptor

- immutable device UUID and descriptor fingerprint (not a claimed serial number unless authenticated); firmware/build and FPGA bitstream identities where present
- `format_ids`, `valid_bits`, `container_bits`, signedness, byte/IQ interleave order, channel count, shared LO group, RX/TX capability distinction
- source rate limits and **actual readback**, center frequency and analog filter information with provenance
- supported codec IDs + versions + input format restrictions, fidelity classes, maximum source/decoded and encoded frame sizes
- memory ceiling and maximum decoder work scratch supported by peer; max active subscribers / rings / CPU/PL resources
- timestamp domains, sample counters, valid UTC reference and uncertainty (or explicitly unknown), channels coherent only if verified
- processing jobs as separately versioned plugins with currently available and busy states, not promises inferred from hardware model

## Clock, continuity and stream model

### Identity

- A **source UUID** identifies an acquisition origin; a **logical channel ID** identifies an RF channel; a **stream ID** identifies one subscribed logical output; an **epoch** is a generation with immutable interpretation of sample order, rate, format, LO/configuration, transform parameters and timebase.
- A **packet sequence** counts transmitted frames within one stream epoch. A **sample index** counts actual complex source samples within that epoch. One does not substitute for the other. For derived products, parent window indices and transform mapping must be described; sample index semantics can differ by product type.
- A **context revision** ensures a frame cannot be decoded under stale metadata. All context/capability data needed by receivers must be sent before frames or otherwise safely resolvable; a receiver missing a required context **must not guess**.

### Continuity

- Source DMA acquisition loss, producer bounded-ring loss, optional codec failure, network packet loss and one slow subscriber are separately metered. Packets may arrive out of order or duplicate on UDP; a sequence tracker must not treat all reorder as samples lost until its declared window closes.
- The source uses a new epoch for rate/retune, hardware reset, oscillator rebase, sample-format or decoding-transform changes and any discontinuity that invalidates index semantics. A known numeric gap can be reported as `[first_missing_sample, count]`; unknown count is an explicit `unknown` value, not zero.
- Receiver must never conceal missing source samples with repeated samples; applications can implement opt-in concealment as a derived data path. Packet CRC catches accidental wire corruption, not spoofing or replay.

### Time

- `sample_index` plus exact rational sample rate gives **relative** sample position only within a valid epoch and declared source timebase; neither implies UTC.
- Timestamps use explicit clock domain ID, units, synchronization method and uncertainty. UTC nanoseconds are only valid with measured time correlation. Dual RX needs evidence of shared clock, matched sample alignment, fixed skew and clock health; rate equality alone is insufficient.
- If clock domains cannot be reconciled, multi-radio fusion consumers need an explicit `unsynchronized` status. Avoid silently aligning independent samples by packet arrival time.

## Codec contract

- Registry ID and version; source sample-format ID, valid bits, packing order; decoded complex sample count and **byte** length; encoded byte count and upper bound; optional codec-side parameters and quantization profile.
- Decoder working memory bound from manifest; max output length checked before allocation; frame integrity check and semantic validation; independently decodable baseline block. Stateful/long-history codecs require explicit checkpoints and chain ID.
- `codec_id=0` always means raw identity payload; `codec_id=1` proposed exact signed CS12 packed I/Q subject to validated input. All other codec IDs require positive negotiation and matching version. Unknown **critical** codecs are rejected, never guessed.
- Mode A exact accepts reversible transforms only. Mode B guarantees numerical max-error bound including any exponent/scale. Mode C exposes declared test-domain metrics, not a universal promise that every future signal meets them. Mode D is non-equivalent derived data and must carry processing provenance.

## Flow control and buffer design

Use a producer-owned bounded capture ring and separate per-subscriber bounded queues; no slow client can block RF DMA or another subscriber. Frame size and jitter buffer policy depend on link/transport. Endpoints signal `PAUSE`, `RATE_LIMIT`, `DROP_WITH_GAP`, `UNSUBSCRIBE` and `UNSUPPORTED` distinctly. A capture that cannot fit must report real drops; an *authorized user* may explicitly renegotiate to a lossy or derived stream with a **new epoch**.

- USB: prefer IP service as first Pluto software-only prototype, retain regular IIO/IIOD. Dedicated composite bulk interface is optional only after device-descriptor/endpoint and multi-host tests.
- TCP/TLS: reliable ordered transport, but HOL latency and slow client behavior need bounding.
- UDP: bounded datagrams and optional FEC/reorder window; no dependence on IP fragmentation. 1280-byte IPv6 minimum path MTU leaves 1232 bytes UDP payload before any extra encapsulation. A conservative **1152-byte complete OrcStream frame** is proposed as a low-MTU profile, subject to real path discovery and tunnels.
- QUIC: suitable for capable hosts/bridges, not baseline mandatory on Tier 0.
- File: record the same headers and contexts with a versioned container/manifest. File is not an excuse to omit epochs or loss records.
- Serial/trusted local: minimally framed data and explicit trust assumptions. A “trusted” cable doesn't automatically authorize sensitive FPGA or TX operations.

## Data products and transformation semantics

| Product | Reconstructs original IQ? | Required metadata |
|---|---|---|
| `RAW_IQ` / `PACKED_IQ` Mode A | Yes for samples actually delivered | native format, valid bits, source index, rate, clock, codec |
| `QUANTIZED_IQ` Mode B/C | No | scale/exponent, precise rounding, error/fidelity profile |
| `CHANNEL_IQ` | No | parent RF band, DDC center, filter coefficients/ID, decimation, group delay, mapping |
| `SPECTRUM` | No | FFT N, window, bin map/width, scaling, calibration, parent span |
| `EVENT` | No | exact measurement inputs, detector/model build, calibration, evidence window, uncertainty and unknown outcome |
| `TRIGGERED_IQ` | Only captured window | pre/post bounds, trigger sample index, retention expiry, overrun/incomplete flags |

For consumer adoption, the SDK should default to **read-only RX**. Independent source adapters may advertise TX only in a future security-reviewed extension. Dynamic FPGA jobs have a reservation lifecycle and busy/unsupported/denied differentiation.

## Security model (nonnegotiable)

- CRC is error detection, **not** cryptographic integrity. Remote config/tune/recording resource control require authentication, authorization and replay-safe session handling over an appropriately secured channel.
- Even read subscriptions can leak sensitive received content and consume device bandwidth; enforce access scope, maximum subscriptions, memory, rates and expiration.
- Frame parser must check `header_len`, payload length, decoded length, count × stride integer overflow, codec params, critical extension types, complexity/work memory and hostile infinite-state/zip-bomb conditions *before* writes or allocation.
- Disconnected peers do not leave DMA, FPGA jobs, GPIO states or active transmit paths unreleased. On reboot/reconnect, capabilities and epoch change; never reuse stale authorization tokens without validation.

## Compatibility and governance

VITA 49/VRT and DIFI define meaningful on-wire standards [A1,A2], so OrcStream should offer standards adapters with explicit mapping-loss tables and conformance tests, not announce VRT/DIFI equivalence. A C99 MIT-compatible SDK goal requires separate license review for third-party codec code, ADI HDL, libiio, OrcSDR decode modules and standards patents. Stable identifiers, binary vectors, registry governance and a reference Wireshark dissector are credible adoption tools.

## Wire proposal and phase gates

Concrete 80-byte v1 discussion draft lives in [`ORCSTREAM_V1_PROTOCOL_PROPOSAL.md`](ORCSTREAM_V1_PROTOCOL_PROPOSAL.md). It is **not** a v1 release or compatibility promise. Freeze only after two independent interoperable decoder implementations, fuzz and corpus roundtrip, resource caps, clock/gap exercises and at least one non-Pluto device source.

**Essential source links:** [A1] https://vita.com/page-1855471 ; [A2] https://dificonsortium.org/standards/ ; [A3] https://developer.analog.com/docs/libiio/main/iiod/ ; [A4] https://uhd.readthedocs.io/en/latest/page_rfnoc_fpga.html ; [A5] https://www.etsi.org/deliver/etsi_ts/103800_103899/103859/07.00.02_60/ts_103859v070002p.pdf ; [A6] https://www.rfc-editor.org/rfc/rfc8200
