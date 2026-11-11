# Protocol evolution proposal: streams, codecs, events and offload

Dated **2026-11-10**; proposed, not implemented. Applies to **all OrcStream sources**, including RTL-SDR, Pluto, PC IQ replay and future FPGA radios.

## Starting point

The unmerged experimental [C99 v0.1 branch](https://github.com/hardcoreerik/OrcStream/tree/chatgpt/protocol-c99-v0.1-20261104) defines a fixed 64-byte big-endian header, HELLO (type 1), IQ (type 2), six explicit formats, sample indices, clock-domain flags, CRC32 and gap tracker. It does **not** support compression, negotiated sessions, events, subscriptions, authentication or device jobs. The draft wire format may change; there is no production promise or v1 compatibility guarantee.

## Evolution constraints

1. Preserve existing experimental code through review, not by silently editing flags with new semantics. Add explicit versioning/extensions or a new draft packet version and update every fixture.
2. Keep original `RAW_IQ` available whenever hardware/transport permits; clients cannot be silently downgraded to packed, quantized, channelized or event-only representations.
3. Maintain separate **acquisition metadata**, **transport metadata**, **transform provenance**, **decoder inference** and **session/control authorization**.
4. Treat hardware and host software as equivalent *capability providers*; driver/vendor bitstream names are metadata, not format semantics.
5. Keep payload encoding bounded and independently implementable by C99, Rust, C++, Python and embedded clients without dynamic allocation requirements.

## Proposed extension families

| Family | Provisional examples | Mandatory semantics |
| --- | --- | --- |
| `CAPABILITIES` | native formats/rates, supported codecs, GPIO/FPGA/decoder jobs | source and firmware identity; limits; unavailable/busy |
| `SUBSCRIBE` | raw IQ, packed IQ, channel IQ, spectrum, events, triggered IQ | profile, resource estimate, explicit fallback policy |
| `IQ` | existing raw IQ and new codec-declared IQ | exact source/format mapping and decompression size/limits |
| `SPECTRUM` | FFT power bins and update cadence | bin coordinates/window, scale, calibration and input sample span |
| `EVENT` | burst, power event, modulation candidate, decoded packet | source sample window, version, measured vs inferred fields, confidence/unknown |
| `CAPTURE` | pre/post-trigger IQ request/result | source epoch, trigger index, buffer depth, overrun and missing-window flag |
| `JOB` | decimator, DDC, FFT, trigger, `phy.vdl2`, `phy.lora` | discover, authorize, reserve, configure, execute, stop, release |
| `TELEMETRY` | source drops, queue drops, thermal, clock state, CPU/FPGA resource | counter scope, monotonic reset/epoch and units |
| `CONTROL` | tune, gain, rate, subscriptions, GPIO | authenticated, authorized, idempotency/replay protection |

Codec negotiation must include algorithm ID/version, native representation, decoded sample count, exact/approximate flag, maximum input/output sizes, expansion bounds, error policy and optional quantitative distortion measures. A codec cannot be called lossless without a byte/sample-exact roundtrip against known source representation.

## Stream and timing model

A stream has a durable logical source/channel identity and a changing **epoch/generation**. Per-packet sequence and first-sample index are distinct; each samples-bearing packet indicates count, native or transformed rate, RF center, sample format and parent source window. Retuning, rate/format/filter/bitstream changes, buffer overruns and interrupted offload must start a new epoch or signal a discontinuity with exact known/unknown gap semantics.

Timestamps carry an explicit **device-monotonic, wall/UTC or other domain** and validity. A counter at the FPGA clock is not inherently UTC. Dual RX requires per-channel identities and declared clock/LO correlation; synchronized channel samples must be proven, not assumed from equal nominal rates.

Derived events may refer to an IQ source window not transmitted to this client. They cannot guarantee that the IQ exists in a replayable buffer. An optional `capture_available` flag must be grounded in actual bounded retention and expiry.

## Source capability / processing-job lifecycle

Proposed state flow:

```text
DISCOVER -> AUTHENTICATE -> REQUEST -> RESERVE -> CONFIGURE
         -> READY -> START -> RUN -> STOP -> RELEASE
                        |             |
                      FAIL          FAULT -> DISCONTINUITY
```

A host may use `host_only`, `source_preferred` or `auto` policy; it selects based on measured bandwidth, compute, power, latency, correctness and resource conflicts. The device reports explicit unsupported, unavailable, busy, auth-required or insufficient-bandwidth results. A change in FPGA bitstream may interrupt acquisition; avoid promises about seamless switching.

Supported optional decoder services should identify protocol/PHY variant, frequency/rate/bandwidth needs, implementation location (FPGA/ARM/P4/PC), CPU/FPGA budget, versions and evidence. Classifications have transparent provenance; unknown labels remain supported.

## Security and integration

Decouple authenticated control from data framing. CRC32 detects accidental corruption, not hostile modification or confidentiality. Initial local loopback demos must not be marketed as safe public listeners. Use secure sessions, per-device permissions, quota-limited subscriptions and safe GPIO/firmware-update policy before remote controls.

Transport adapters include TCP, UDP, file and optional USB bulk/USB IP; independent standards bridges (rtl_tcp, VITA-49, SoapyRemote and others) must have protocol-conformance tests. No vendor-specific wire dialect.

## Decision gates before wire freeze

- Review C99 v0.1 protocol header against sample count/format/timebase and portable implementations.
- Choose extension framing/registry, negotiation versioning and codec extensibility with fixtures.
- Validate exact raw and reversible-packing roundtrip using deterministic and live IQ.
- Independently exercise at least two source families and three host classes.
- Demonstrate multi-client backpressure and secure control without acquisition stalls.
- Document interoperability benchmark method, resource and timing results.

See [../ROADMAP.md](../ROADMAP.md) and [PLUTO_ACCELERATION.md](PLUTO_ACCELERATION.md).
