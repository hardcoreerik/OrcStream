# OrcStream

**OrcStream is a standalone, open SDR data and control protocol and portable SDK project.** It aims to connect radio hardware, software, firmware, computers, gateways and embedded devices without tying the protocol to one application or device.

**Goal:** preserve original IQ when requested, while optionally delivering reversible packed IQ, filtered channels, spectrum, events, triggered samples and processing offload **when a source actually supports them**. Fidelity, sample timing and loss must remain explicit.

## Status

**Research and architecture stage on `main`.** An [experimental C99 v0.1 codec](https://github.com/hardcoreerik/OrcStream/tree/chatgpt/protocol-c99-v0.1-20261104) exists on a separate unmerged branch; it is not a released protocol or stable API. Compression, subscriptions, security, FPGA jobs and real Pluto integration are not yet implemented.

## Project tracking

- [PROJECT_TRUTH.md](PROJECT_TRUTH.md) — canonical current truth, scope and non-negotiable principles
- [ARCHITECTURE.md](ARCHITECTURE.md) — independent layers, hardware adapters and proposed offload model
- [ROADMAP.md](ROADMAP.md) — workstreams, milestones and acceptance gates
- [STATUS.md](STATUS.md) — evidence ledger and next engineering step
- [Pluto acceleration plan](docs/PLUTO_ACCELERATION.md) — optional compatible firmware/FPGA reference platform
- [Protocol evolution](docs/PROTOCOL_EVOLUTION.md) — proposed streams, codecs, events and processing-job contracts
- [Validation plan](docs/VALIDATION.md) — repeatable fidelity, security, performance and hardware acceptance

OrcSDR and [`esp-pluto-sdr`](https://github.com/hardcoreerik/esp-pluto-sdr) are intended early consumers, **not requirements**. Stock Pluto IIO compatibility and independent drivers are deliberate architectural constraints. The long-term goal includes Windows/Linux/macOS, Raspberry Pi and ESP32-P4 hosts as well as other SDR hardware.

**Built out of spite. Engineered to be undeniable.** Performance and interoperability claims must be earned with reproducible evidence.
