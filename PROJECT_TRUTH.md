# PROJECT_TRUTH — OrcStream

Authoritative project truth as of **2026-11-10**. This file overrides roadmap language about what is available today. Claims require named revisions, tests, and platforms. See [STATUS.md](STATUS.md) for the changing evidence ledger.

## Overall goal

Build **OrcStream**, a standalone, open and device-agnostic protocol and portable SDK connecting SDR hardware, firmware, computers, gateways and applications. Any source must be able to offer *original, fidelity-preserving IQ*; capable sources may additionally advertise reversible compression, filtered/channelized IQ, spectrum, event detections, triggered captures, decoder results and hardware offload. All transformations must be explicitly requested and described.

The first intended adopters include **custom PlutoSDR firmware**, **OrcSDR** and other ESP32-P4/PC/Raspberry Pi clients. They are **consumers, not dependencies**. OrcStream cannot require a Pluto, FPGA, ESP32, OrcSDR or a particular operating system.

**Core promise:** original samples when requested; optional efficient representations when negotiated; truthful timestamps, signal evidence, classification and loss reporting. "Built out of spite. Engineered to be undeniable" is an ethos, not a benchmark claim.

## Evidence labels

| Label | Meaning |
| --- | --- |
| MERGED | Available in this repository's `main` branch |
| EXPERIMENTAL | Implemented on a named unmerged branch; not a stable release |
| REPORTED | Dated test claim in a separate project or branch, not independently retested here |
| PROPOSED | Accepted roadmap direction; not implemented |
| DEFERRED | Deliberately outside current milestone |
| BLOCKED | Dependency or verification missing |

## State at this snapshot

- **MERGED:** `main` contains a research-stage README and these project-planning documents after this documentation PR is merged. It does **not** contain a shipped OrcStream protocol library.
- **EXPERIMENTAL:** [`chatgpt/protocol-c99-v0.1-20261104`](https://github.com/hardcoreerik/OrcStream/tree/chatgpt/protocol-c99-v0.1-20261104) implements a portable C99 packet codec with a 64-byte experimental wire header, HELLO and IQ frames, CU8/CS8/CS16/CF32 formats, 64-bit indices, timestamp-domain flags, CRC32, bounded parsing, file/UDP examples and deterministic host tests. It is **not merged** or a frozen specification. No device integration or fidelity/compression superiority is established.
- **REPORTED:** [`esp-pluto-sdr` `claude/iiod-client-core`](https://github.com/hardcoreerik/esp-pluto-sdr/tree/claude/iiod-client-core) documents Fishball/Zynq Pluto USB/IIOD IQ receiving near 11.5 MB/s at 2.88 MS/s; [`OrcSDR` `claude/pluto-p4-core-source`](https://github.com/hardcoreerik/OrcSDR/tree/claude/pluto-p4-core-source) documents hardware IQ reception and P4 integration. Neither establishes an OrcStream producer on Pluto.
- **PROPOSED:** An optional specialized Pluto Linux/FPGA firmware (new repository to be decided) exporting OrcStream along with conventional IIO and a generic host driver. Neither native OrcStream USB output nor FPGA OrcStream codecs/decoders are implemented.
- **NOT IMPLEMENTED in OrcStream main:** sessions, subscriptions, compression, live hardware sources, network fanout, security, classification, processing jobs, GPIO controls, USB transport or device-agnostic acceleration.

## Non-negotiable project principles

1. **Standalone protocol and SDK:** no required OrcSDR headers, UI, radio model, vendor firmware, USB stack or application dependencies. Keep source drivers and transports separate.
2. **Preserve raw IQ:** declare native format, byte order, source representation, frequency, rate, channel and timing. `RAW_IQ` never silently converts, drops, decimates or quantizes. Link failure must surface as loss/error, not a hidden fallback.
3. **Do not confuse compression with reduction:** reversible packing may be exact; filtering, decimation, CS8 conversion, event-only reporting and decoder results cannot recreate the original wideband IQ.
4. **Account for real loss:** independent source overrun, producer queue, network loss, subscriber loss, reset and handoff counters; generation/epoch changes when continuity is invalid.
5. **Truthful data:** FPGA/sample counters are not UTC without independent synchronization. Spectral features are measurements or estimates; protocol/modulation identities are hypotheses until validated. Unknown classification is acceptable.
6. **Optional acceleration:** any client can work without FPGA. The Pluto may expose filtering, dual RX, triggered capture, spectrum, GPIO, VDL2 or LoRa services *if actually installed and available*. No capability is inferred from device branding.
7. **Stock compatibility:** Pluto companion firmware should preserve standard IIO/IIOD raw IQ and ordinary hardware controls. OrcStream is an **opt-in** native output, not a replacement.
8. **Portable hosts:** PC, Linux/Raspberry Pi and ESP32-P4 adapters; Ethernet, TCP, USB networking or dedicated bulk are implementations of the same protocol, not separate dialects.
9. **Safe controls:** authenticated and authorized remote writes; immutable device/firmware identity; explicit resource reservation for conflicting LO/DMA/FPGA functions; RX-first and no accidental transmitter controls.
10. **Interoperability by test:** rtl_tcp, VITA-49/VRT, Soapy/SDR++ clients through explicit adapters/conformance suites. No equivalence or performance claims without independent benchmarks and IQ fidelity evidence.
11. **Licensing:** preserve OrcStream SDK's independent permissive-code goal; review Fishball, libiio, FPGA, decoder and other upstream licenses before reuse. Do not automatically relicense firmware derivatives as MIT.
12. **No fabricated release state:** this is a plan. Update this document only after merging and verifying code or hardware.

## Two coupled but independent development tracks

**OrcStream track:** generic wire specification, C99 SDK, negotiation, optional codecs, integrity/continuity metadata, streams/events/jobs, security, transport adapters, tests and adoption.

**Pluto reference track:** portable `esp-pluto-sdr` hardware driver plus separately maintained specialized Pluto firmware/FPGA: standard IIO fallback, optional native OrcStream, hardware processing, GPIO, events and decoder offload. OrcStream defines transferable semantics but must remain useful without the Pluto.

See [ARCHITECTURE.md](ARCHITECTURE.md), [ROADMAP.md](ROADMAP.md), [STATUS.md](STATUS.md), [docs/PLUTO_ACCELERATION.md](docs/PLUTO_ACCELERATION.md), [docs/PROTOCOL_EVOLUTION.md](docs/PROTOCOL_EVOLUTION.md), and [docs/VALIDATION.md](docs/VALIDATION.md).
