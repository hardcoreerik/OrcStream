# OrcStream Engineering Roadmap — Agent 3 research-to-implementation plan

**Status:** proposal, no repository changes or branch merges authorized by this research run. 2026-11-21. This extends—not supersedes—[`OrcStream/ROADMAP.md`](https://github.com/hardcoreerik/OrcStream/blob/66c3a294d36c2ad1b38243b9146952cabb5aa22f/ROADMAP.md).

## Guiding constraints

- The Pluto-compatible Zynq-7020/AD9361 source is the **reference implementation**, never a mandatory OrcStream dependency.
- Maintain stock IIOD/libiio and an independently versioned optional Pluto firmware service. Preserve known-good boot/recovery artifacts.
- **Evidence gates > deadlines.** Every performance, fidelity, interoperability, security and hardware claim requires reproducible tests and named revisions.
- Source-specific silicon/board facts belong to `OrcPluto-Firmware` and associated driver repos; universal codec/wire/SDK belongs to `OrcStream`; UI-specific consumption belongs to `OrcSDR`.
- A protocol draft is not shipped software. The current main branch contains architecture docs, while unmerged C99 v0.1 contains early packet code [R1,R2].

## Stages and pass/fail evidence

| Stage | Work package and owner | Exit criteria | Hard stop |
|---|---|---|---|
| **R0 — research baseline (this package)** | Source audit; competitive map; 16 codec methods; concrete wire proposal; corpus definition; synthetic probe | Eight named docs, reproducible size probe and transparent unsolved questions | No “measured Pluto speed” or market-leadership claims |
| **P1 — C99 baseline conformance** | `OrcStream`: review v0.1 branch independently, implement nominated v1 experimental wire in a **separate dev branch after authorization**, decoder/test vectors, memory limits | Cross-compiler C99 roundtrips CU8, CS16_LE/BE, CF32 binary samples; fuzzer and sanitizer runs, epoch/context gaps; 2 independent decoders | No merge or wire freeze before spec consensus / version negotiation |
| **P2 — source-neutral exact codec pack** | `OrcStream`: CS12 exact pack/unpack; I/Q/endianness; raw fallback and feature negotiation | Exhaustive/extrema tests, CRC and size checks; independent raw→packed→raw hashes; no false signed12 assumptions | Any truncated/out-of-range source sample without explicit error |
| **P3 — comparative lossless codecs** | `OrcStream`: Rice, delta/predictor, LZ4, zstd and raw/pack selector, measured costs | 2+ source families, 20+ scene types/conditions, matched quality, p99 deadline and energy budget; worst-case bypass proven | A codec that cannot bound memory, expansion or receiver requirements |
| **P4 — fidelity-controlled codec** | `OrcStream`: BFP8/10/12, block-length study, deterministic fixed-point VHDL/Verilog/C comparison | Bound verified, weak-signal/phase/BER/PER and PSD regression; same decoder output on CPU/RTL; explicit opt-in | Uncalibrated loss or silent quality downgrade |
| **P5 — independent desktop/embedded adoption** | Generic C99 SDK, file and UDP/TCP, Soapy/GNU Radio, ESP32-P4 builds | Two independent apps + one MCU-class receiver; hardware source diversity, 2-client backpressure tests and Wireshark vectors | Core source requires Pluto headers or OrcSDR app code |
| **P6 — Zynq software reference** | `OrcPluto-Firmware`: board reconnaissance, source-native producer on Zynq Linux + stock IIO | Exact board/recovery identification, source CS16 proof, network IQ hash, CPU and drops, IIO coexistence | Any firmware image changes without boot recovery / baseline verification |
| **P7 — FPGA acceleration** | `OrcPluto-Firmware`: packer → BFP RTL → gated, separate DDC/FFT event path | FPGA resource/timing closure, bit-exact reference vectors, measured DMA/source/transport rates; robust codec fallback | Assuming board-specific bitstream feature merely because AD9361 exists |
| **P8 — interoperability and security** | `OrcStream` adapters; transport and secure session services | DIFI 1.3/VRT mapping fixtures, Soapy / GNU Radio use, authenticated config, quotas, fuzz/replay, cross-device smoke | Public remote-write controls without appropriate authentication |
| **P9 — release candidate / governance** | Open vendor-neutral codec registry, conformance suite, documented semver | Third-party decoder implemented from docs, CI on host targets, license/IPR review, source/release provenance | Incomplete ABI or mislabeled “stable v1.0” |

Stages P2–P5 can be parallelized after P1 core format boundaries stabilize. Zynq/FPGA stage work is **not** a blocker to generic OrcStream adoption.

## Three immediate experiments — detailed plans

### Experiment 1 — exact sign-extended 12-bit IQ feasibility

**Question:** Does the real AD9361/IIO data path on the exact Fishball board deliver a consistent, losslessly packable signed12 format across its supported rates/channels?

**Preconditions:** board revision, kernel/bitstream/firmware, native IIO scan descriptors, source channel order and sign-extension rules recorded; safe recovery verified. **Stimuli:** zero/terminated antenna, calibrated CW/gain sweeps, min/max generated digital fixtures, dual RX if physically wired and enabled. **Procedure:** capture CS16 unmodified; check `(x >> 11)` sign behavior and range, endian/scan order; pack12 then unpack12; compare SHA-256 and original samples per chunk. **Outputs:** source manifest + exact sample hashes, fail cases, maximum capture rate **with zero drops**, CPU and bus data. **Stop:** any out-of-range or unverified sample layout; no lossless pack promise for that path.

### Experiment 2 — adaptive lossless selection at bounded deadlines

**Question:** Can a codec selector beat *every fixed candidate* on a predeclared mixed weighted corpus while never misrepresenting exactness and respecting p99 deadline/memory?

**Comparison:** raw16, exact packed12, block delta+Rice, LZ4 packed12, zstd packed12 and per-block policy; same blocks 1–16 KiB, source data, fidelity and network target. **Corpora:** uniform noise, receiver Gaussian noise, AM/FM/CW/FSK/PSK/QAM/OFDM, weak+strong, burst and high-occupancy real captures. **Success:** deterministic bypass on incompressible blocks, no expansion beyond stated frame overhead, lower weighted wire total and acceptable p99 than selected baselines; report CPU/RAM. **Stop:** selector cost causes deadline or queue overflows, or regression is only hidden by prefiltering/downsampling.

### Experiment 3 — BFP fidelity and FPGA suitability

**Question:** Can a low-cost integer BFP encoder provide a better useful-rate/fidelity tradeoff than fixed-width truncation without compromising weak-signal/coherent uses?

**Matrix:** block sizes 16/64/256, mantissa 8/10/12, shift and rounding variants, actual time-domain vs post-channelizer IQ; software golden model, Verilator/cocotb or RTL testbench, then PL timing/resource profile. **Measurements:** guaranteed max error LSB, reconstruction EVM/PSD, weak-signal Pd at fixed Pfa, demod BER/PER across SNR, inter-channel phase/lag, LO drift plus actual transmit/network drops. **Success:** all declared numerical bounds pass; selected profiles meet pre-registered application criteria with FPGA timing closure and no IIO regression. **Stop:** weak-signal loss masked by aggregate EVM or model/RTL disagreement.

## Branch and repository handoff

**No commit/push was made during this research phase.** Suggested authorized future paths:

- `OrcStream`: `docs/research/2026-11-agent3/` for eight reports; `tests/iq_codec_bench/` for synthetic script/data manifests; `include/orcstream/`, `src/` and `tests/` only after a separately authorized implementation phase.
- `OrcPluto-Firmware`: board-specific sample format manifest, stock IIOD coexistence test, FPGA acceleration module and signed artifact provenance. Leave protected `main` untouched.
- `OrcSDR`: optional receive source/plugin, UI fidelity/clock/gap labels, no codec semantics owned by application.
- SDK compatibility: do **not** silently cherry-pick v0.1 into main or reinterpret reserved v0.1 fields. Keep v0.1 fixtures and explicitly note incompatible v1 draft.

## Proposed cadence / decision gates

1. **Architecture freeze gate:** review proposed 80-byte header and TLV definitions, disagreement log, two external reviewers, resource budget evidence.
2. **Codec registry gate:** raw and exact CS12 IDs, normative signed arithmetic, gold vectors, roundtrip and error behavior; `v1` label remains draft until adopted.
3. **Interoperability gate:** two implementations and at least one non-OrcSDR consumer on two hardware-source families.
4. **Fidelity gate:** complete modes and independently validated lossy metrics, source provenance and gap accounting.
5. **Security gate:** remote session authn/authz and quotas before off-device configuration endpoints.
6. **Hardware release gate:** documented board boot/recovery, output goodput, IIO coexistence and FPGA resource proof.
7. **Governance gate:** permissive-source license compatibility, standards/patent review, stable ABI, test corpus redistribution rights.

## Dependencies, critical risks, and mitigations

| Risk | Severity | Mitigation |
|---|---|---|
| Mode A over low-bandwidth link mathematically impossible | Critical | immediate admission control; user chooses lossy/derived independently or receives explicit failure |
| Wrong signed12 assumption | Critical | IIO scan metadata and exhaustive range/hash proof before encoding |
| Misleading frequency-domain O-RAN generalization | High | time-domain control experiment and independent error contract |
| Weak signal loss below strong interferer | High | weak detection and BER tests, enforce Mode B/C visibility |
| Variable codec CPU causing DMA overrun | High | source isolation, per-block budget/bypass, capture ring stats |
| Stale context after retune/bitstream reload | High | immutable stream epoch and required context revision |
| USB gadget customization breaks stock support | High | IP-first producer and known-good recovery, Linux/Windows/P4 interop matrix |
| Security: CRC mistaken for authentication | Critical | authenticated control plane, untrusted frame caps, replay and quota tests |
| Third-party codec license/patent issue | High | source inventory, no automatic relicense, expert review before product release |
| Branch/release confusion | Medium | explicit experimental labels; named commit hashes and repository ownership |

## Mandatory research questions answered provisionally

1. **Best rate/fidelity/compute balance:** verified CS12 packing for exactness; BFP for low-cost explicitly lossy profiles; adaptive Rice+raw bypass for promising lossless FPGA-oriented research. No universal winner yet.
2. **Best FPGA candidates:** bit packing, predictors/Rice, BFP fixed-point and optional source-side channelizer. zstd/general ANS less suitable as the first PL blocks.
3. **Adaptive hybrid outperform all fixed:** possible on a mixed, skewed corpus in *wire size*, not guaranteed on all input strings and subject to CPU/latency. Requires experiment 2.
4. **Small embedded:** identity, packed sample bit operations, simple delta/Rice, optional bounded BFP decode; not obligatory neural/zstd.
5. **Real AD9361 ratios:** unknown; measured real captures from exact firmware needed. A padded16→packed12 exact representation reduces bytes by 25% *if valid sign-extension proof exists*.
6. **USB2 relief conditions:** known redundant bits, true source entropy below capacity, permitted bounded-error quantization, or intentionally derived/channelized products. Need actual measured goodput and rate control.
7. **Impossible without error:** universal lossless shortening of arbitrary incompressible IQ; full-band restoration from filtered/decimated IQ and events without additional information.
8. **Why vendors adopt:** bounded C99 SDK, open vectors, portable backends, explicit fidelity/clock/gaps, secure sessions, standards adapters and meaningful goodput evidence.
9. **First three experiments:** signed12 real source proof; bounded adaptive selector; fixed-point BFP fidelity/RTL.
10. **Hard to change after v1:** header field layout, codec/sample-format IDs and semantic versioning, sample counter/epoch/timebase, framing limits, extension policy, quantizer rounding and sign conventions, control authorization and context mapping.

## Reference links

- OrcStream source truth: https://github.com/hardcoreerik/OrcStream/blob/66c3a294d36c2ad1b38243b9146952cabb5aa22f/PROJECT_TRUTH.md
- OrcStream v0.1 wire: https://github.com/hardcoreerik/OrcStream/blob/27c134060631a2efb74da04ee760e49957474d96/docs/wire-v0.1.md
- Analog Devices AD9361 Linux driver: https://developer.analog.com/docs/linux/drivers/iio-transceiver/ad9361/
- ETSI O-RAN BFP: https://www.etsi.org/deliver/etsi_ts/103800_103899/103859/07.00.02_60/ts_103859v070002p.pdf
- DIFI standards: https://dificonsortium.org/standards/
