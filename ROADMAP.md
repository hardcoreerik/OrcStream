# ROADMAP — OrcStream

Dated plan **2026-11-10**. Milestone gates, not promises of released features. See [STATUS.md](STATUS.md) and [PROJECT_TRUTH.md](PROJECT_TRUTH.md).

## Completion goal

An independent, documented protocol/SDK with **bit-preserving native IQ**, optional explicit efficient streams/processing jobs, real session security and reliability semantics, and audited adapters for PC/Linux/Raspberry Pi/ESP32-P4. A custom Pluto firmware/FPGA reference must demonstrate optional OrcStream-over-USB or IP alongside ordinary IIO and evidence-backed offload; other radios must remain first-class.

## Workstreams

- **A: Universal protocol/SDK (critical path):** framing, fidelity, portable hosts, sessions, codecs, subscriptions, metadata, transport.
- **B: Pluto reference accelerator:** independently maintained `esp-pluto-sdr` driver and separate Pluto firmware/FPGA, with IIO fallback.
- **C: Cross-platform adoption:** Linux/Windows/macOS, Raspberry Pi, ESP32-P4, OrcSDR and independent consumers.
- **D: DSP/event/decode:** FPGA filtering, dual RX, GPIO trigger/capture, spectrum, classification, VDL2/LoRa optional services.

## Phases with exit criteria

### P0 — Authoritative plan and truthful status

Deliver `PROJECT_TRUTH.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `STATUS.md`, `docs/PLUTO_ACCELERATION.md`, `docs/PROTOCOL_EVOLUTION.md` and `docs/VALIDATION.md` on `main`. Document experimental branches without merging unfinished implementations.

**Exit:** docs merged and cross-linked; clear independent-project boundaries and documented unmet hardware gates.

### P1 — Byte-exact portable IQ codec [HIGHEST INITIAL VALUE]

In a **separate sandbox development branch**, inspect the existing `chatgpt/protocol-c99-v0.1-20261104` experiments; do not overwrite or silently merge. Reconcile/design review with `main` and bring the smallest proven C99 codec, host tests and test vectors forward. Reproduce CMake/CTest on Linux, Windows/macOS where available; ASan/UBSan and malformed/fragmentation fuzz tests; ESP-IDF esp32p4 compile smoke and bounded allocation/cycle profile. Extend unambiguous sample format, loss/epoch and rate metadata only with versioned tests.

**Exit:** packet roundtrip byte-for-byte for all advertised formats; documented header/protocol test vectors, strict parser and error codes, actual recorded CI/test outcomes and current limitations. No wire freeze or superiority claim yet.

### P2 — Sessions, subscriptions and security

Design and implement capability negotiation, subscription lifecycle, stream families, source/consumer backpressure, typed metadata and control authorization. Add TCP and local UDP transport adapters and explicit gap/retransmit policy where appropriate. Support slow-consumer isolation, reconnect and stream epoch transitions.

**Exit:** two independent clients subscribe to source IQ without corrupting each other's continuity; malformed/disconnected clients do not stall acquisition; control writes are authenticated and rate-limited; network impairment tests record measurable loss and latency.

### P3 — Raw IQ adoption and compatibility

Integrate one independent RTL/OrcSDR producer, a native CS16 source from Pluto driver, desktop receiver, Raspberry Pi and ESP32-P4 host client. Add rtl_tcp and VITA-49/VRT adapters as separate, tested modules; explore Soapy and SDR++ bridges. Keep raw sample format unchanged.

**Exit:** IQ from at least two hardware-source families reproduces correctly at client; capture files and hashes prove sample order/fidelity; benchmark format, CPU, memory, end-to-end rates and discontinuity counters on named setups.

### P4 — Optional native Pluto OrcStream producer

Establish separately versioned Fishball baseline/recovery. Run a minimal OrcStream producer on Pluto Zynq Linux using Ethernet and/or existing USB IP if validated. Keep stock IIO path operational. Negotiate raw IQ source format and stream epoch. Add PC/Pi/P4 host support; assess safe dedicated USB bulk gadget function later.

**Exit:** independent client receives truthful CS16 IQ from Pluto without OrcSDR; byte/sample checks, rate tests and hotplug/recovery evidence; standard libiio still operates after mode change.

### P5 — Hardware-assisted compact streams and GPIO

Implement FPGA-side decimation/filtering (verify actual DMA-output rate), benchmark reversible packed-12 IQ separately from lossy CS8, add feature/event streams with source sample references. Add GPIO safe control, trigger/capture pre/post windows and bounded DDR storage. Verify available resources, endian and sign extension.

**Exit:** filter response/alias rejection measured against software reference; reversible IQ packing roundtrips bit-perfect; measured USB/IP bandwidth and CPU/FPGA resource use; GPIO trigger tagged to known sample indices and safe disconnect/boot tests.

### P6 — FPGA acceleration and processing-job scheduling

Implement optional generic channelizer, spectrum/FFT/power events and modulation characterization. Formalize `discover/authorize/reserve/configure/start/stop` jobs, shared-LO/bandwidth/resource conflicts, source selection and fallback across P4/PC/Pi. Experiment with optional VDL2 channelization/decoder and LoRa dechirp/detection/decoder only after reference corpora and licensing review.

**Exit:** hardware job availability is accurately discovered; busy/unsupported/missing firmware cases are safe; known-signal false positives/negatives and packet decode parity measured; sample gaps across handovers explicitly reported.

### P7 — Stable release candidate and outside adoption

Threat model/control hardening; interop and API version compatibility, complete SDK docs/examples, reproducible FPGA/firmware builds and signed/update rollback guidance where possible; sustained stress across varying host hardware; independently replicable comparisons against reference protocols. Declare wire v1 only after real consumers demonstrate required compatibility.

**Exit:** stable documented release policy, CI and device acceptance matrix, firmware rollback, reproducible evidence, a third-party-consumable public SDK and examples.

## Milestone dependencies / priorities

P1 is the immediate OrcStream repo work; **P2–P3** prove the protocol independent of the Pluto. The firmware baseline for P4 can be researched **in parallel** but must not block P1. P5 builds on verified P4 and DSP truth. P6 assumes source metadata, transport/epoch semantics and validated reference signals. Avoid early FPGA neural networks, unsupported TX, or premature marketing benchmarks.

## Risks and gates

- USB endpoint configuration and cross-platform USB network-class support: prove with hardware and host tests; fallback to Ethernet/IIO.
- Zynq CPU/FPGA fabric capacity: quantify LUT/BRAM/DSP, thermal, DMA, PS/PL, memory, clock margins and real throughput.
- Decimator/CS12 handling: verify actual source scan format, signedness, filter response and provenance.
- Dual-RX/shared LO and simultaneous jobs: capability-gated scheduling, not assumed independence.
- Security of remote control/firmware updates: fail closed, authenticated writes; do not confuse CRC with authentication.
- Source-side event-only output: no claim that samples not captured can be recovered later.
- Firmware and decoder licensing: keep reusable OrcStream MIT core distinct from third-party/copyleft derivatives and mark provenance.

## Update process

Record every meaningful milestone in [STATUS.md](STATUS.md): date, branch/commit, evidence and remaining gaps. Update PROJECT_TRUTH after a state transition, not just because a branch exists. Project-specific implementation details live in [docs/PLUTO_ACCELERATION.md](docs/PLUTO_ACCELERATION.md) and protocol proposals in [docs/PROTOCOL_EVOLUTION.md](docs/PROTOCOL_EVOLUTION.md).


## P1 review checkpoint (2026-11-10; unmerged)

The `agents/p1-c99-fidelity-core` branch demonstrates repeatable native IQ host fidelity and negative-case tests; see [P1 validation](docs/P1_VALIDATION_REPORT.md). This is **not the P1 release gate**: observed Windows/macOS CI, ESP-IDF esp32p4 compile, expanded adversarial corpus, independent third-party wire decoding and device tests remain open. No wire/API freeze.
