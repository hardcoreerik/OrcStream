# OrcStream Codec Portability Matrix — Agent 3

**Status:** proposed device-tier support, **not** tested platform performance. 2026-11-21. Requirements are engineering estimates unless specifically marked protocol-level commitments. Resource tests are future acceptance gates.

## Device tiers and recommended profiles

| Tier | Representative target | Mandatory baseline recommendation | Optional codec candidates | Decoder RAM/CPU policy | Encoder hardware direction | Known proof gap |
|---|---|---|---|---|---|---|
| **0: minimal embedded** | Small MCU, low-power RF module | Identity RAW (for advertised native formats); bounded frame parsing; 8/12-bit exact packing where valid | fixed-Rice, tiny block delta, MCU-safe BFP decode if negotiated | **No heap required**; caller-supplied frame buffer; 512–4,096-byte block target for early experiments, not fixed requirement | shifts/masks; no general entropy codec dependency | measured cycles, stack, smaller RAM, endianness |
| **1: capable embedded** | ESP32-P4, ARM Cortex-M7/A7 class | baseline + format conversion only on **explicit request**, per-epoch context | BFP8/10, Rice, LZ4 decode, simple LZ4 encode; bounded-state predictors | profile small and large bounded windows; cap scratch via negotiated manifest | intrinsics/DMA, bitpacking, fast integer loops | test ESP-IDF P4 compiler/memory/USB and PSRAM DMA paths |
| **2: ARM + FPGA SDR appliance** | Zynq-7020/AD9361 / future OrcNode | baseline + packed CS12 on verified native source | BFP, Rice/bitplane, zstd/LZ4 on ARM when ROI proven, RTL accelerators | isolate DDR rings from codec scratch; bounded PS/PL DMA | FPGA bit packing, block max & shift, FIR/DDC/FFT, independent host framing | actual bitstream DSP/BRAM/timing, ARM saturation, IIO coexistence |
| **3: host CPU** | x86-64/ARM64 PC/Pi/server | baseline + broad receiver SDK | zstd, LZ4, FLAC experimental, ANS, hybrids, BFP | bounded ring + SIMD, multi-core; avoid uncontrolled allocations and HOL | parallel block encoders, SIMD packing and predictors | repeated CPU/latency benchmarks and portable CI |
| **4: acceleration** | GPU, large FPGA, 10G+/multi-radio | baseline + multi-lane negotiated high-rate codecs | vector rANS, learned predictor research, multi-channel transform and BFP, FPGA line coding | pinned buffers and explicit transfers; no codec backpressure into acquisition | AXI-Stream, LUT/DSP/BRAM, DMA scatter/gather, GPU only above batch break-even | perf per watt, bus overhead, deterministic decode, coherence |

**Important:** a Tier 0 sender never has to decode a Tier 4 stream. Data direction, supported formats and codec profiles are negotiated separately; minimum parsing does not require shipping every codec encoder/decoder. The proposed universally mandatory **wire** codec is identity (`codec_id=0`), not zstd or any neural model. That decision is essential for open adoption.

## Codec / Tier matrix

Legend: **M** baseline mandatory support *when an advertised source format is negotiated*; **Y** practical early option; **X** research option; **—** not recommended as an embedded requirement. No performance claim from a checkmark.

| Algorithm or feature | T0 | T1 | T2 | T3 | T4 | Dominant concern |
|---|:---:|:---:|:---:|:---:|:---:|---|
| Identity/raw framing and strict bounds | M | M | M | M | M | CRC + packet rate + bounded frame parsing |
| Validated CS12 reversible packing | Y | Y | Y | Y | Y | Source-valid-bits proof, bit order, sign extension |
| Byte/bit shuffle | X | Y | Y | Y | Y | Transpose buffer and memory traffic |
| Delta / fixed predictor | Y | Y | Y | Y | Y | Reset/checkpoint, noisy data fallback |
| Adaptive LPC | — | X | X | Y | Y | State and coefficient side information |
| Rice/Golomb | X | Y | Y | Y | Y | Worst-case unary escape, decoder exactness |
| Canonical Huffman | X | Y | Y | Y | Y | Code table validation + RAM |
| rANS | — | X | X | Y | Y | Tables, reverse coding and state complexity |
| LZ4 | — | Y | Y | Y | Y | Memory window, no expansion guarantee |
| zstd | — | — | X | Y | Y | Decoder RAM and sustained compute |
| Block float integer m=8/10/12 | X (decode) | Y | Y | Y | Y | Error contract, block strength sensitivity |
| Block scaling / companding | — | X | Y | Y | Y | Scale and calibration accuracy |
| FFT/subband and DDC products | — | X | Y | Y | Y | *Derived products*, not original IQ |
| Cross-channel decorrelation | — | — | X | X | Y | Coherence, samples aligned before transform |
| Learned autoencoder | — | — | X | X | X | Model compute and unbounded fidelity risk |

## Memory and frame design

- **Nominal experimental baseline:** `max_decoded_bytes=4096`, `max_encoded_bytes=4096` (or smaller negotiated value) for Tier 0; choose a memory cap **per endpoint**, not globally in the wire format. The 80-byte proposed frame header consumes extra space; keep total smaller than physical link frame limits. These are design starting values, not a validated hardware promise.
- For a native CS16 stream, 4 KiB decoded output corresponds to 1,024 complex samples, and 3 KiB packed12 output if exact12. A decoder needing only one 4 KiB output buffer plus bounded 1–4 KiB input staging is conceivable for Tier 0, but **actual stack/heap and max live-buffer counts must be measured**.
- For BFP with 256-sample block, 8-bit I/Q mantissas require 512 data bytes plus one exponent byte (16.03125 bpc) before protocol frame; no need to allocate full 16-bit source in a streaming encoder if FPGA max-and-replay hardware has a two-pass buffer/ring.
- For Rice with block residuals, use a capped escape length or separate raw blocks; a memory cap does not by itself bound CPU if unary code words can be arbitrarily long.
- Multi-client source needs single acquisition ring and per-consumer bounded queues, rather than one buffering path for every subscriber under DMA lock.

## Hardware path recommendations

**Tier 0:** pack and stream known-native source data. Stateless per-block decode with explicit output sizes. Use compile-time arrays and no allocator in core.

**Tier 1 ESP32-P4:** DMA/USB host throughput and Wi-Fi link capacity may dominate. Favor exact packing and low-compute decode; keep compression on source side where possible (e.g. Zynq FPGA) instead of transferring the full raw stream first. Any conversion by P4 is a *host gateway* and cannot reclaim upstream USB bandwidth.

**Tier 2 Zynq-7020:** RF source → AD9361 HDL/IIO DMA → verified sample format → FPGA pack/BFP when instantiated and benchmarked → DDR ring → ARM Linux producer/network. Start with software-only source-native producer alongside IIO; introduce PL accelerator only after board revision, kernel, HDL and recovery plan are known. No assumption that every Fishball clone uses the same bitstream, DDR or USB gadget configuration.

**Tier 3/4:** decode broad codec registry and choose SIMD/parallel kernels without changing wire. A GPU is not useful for every 1 ms packet: host/device transfer overhead may exceed compute. Compare latency at equivalent fidelity and block size.

## Portability/acceptance checklist

1. Build C99 SDK on Linux GCC/Clang, Windows MSVC/MinGW, macOS Clang, ESP-IDF P4 and ARM cross compiler (not assumed passed).
2. Golden vector exactness at extrema `[-2048,-1,0,2047]` for CS12, plus invalid-source refusal on out-of-range CS16.
3. Cross-platform decode checks at every size boundary, endian direction, malformed header/CRC/length, invalid codec and integer overflow.
4. Use opaque frame syntax across all backends; CPU SIMD and FPGA RTL must decode to the **same exact samples** or same declared lossy error bound.
5. Per-tier benchmark outputs: C compiler flags, CPU clocks, stack peak, heap peak, cycles/sample, encoder/decoder wall time, power, temperature, block p99 and recovery behavior.
6. Validate packet loss and new-epoch resets, especially stateful codec reset; no stale coefficients on reconnect.
7. Dual-channel coherence regression and weak-signal false-negative testing before BFP can be enabled on a measurement profile.
8. Hardware output should be bitwise self-described; do not need vendor SDK to parse archived OrcStream frames.

## External precedents (not performance endorsements)

- AD9361 12-bit native and rate constraints: https://developer.analog.com/docs/linux/drivers/iio-transceiver/ad9361/
- O-RAN BFP and block scaling algorithms: https://www.etsi.org/deliver/etsi_ts/103800_103899/103859/07.00.02_60/ts_103859v070002p.pdf
- FPGA fronthaul compression examples: https://docs.altera.com/r/docs/683238/25.1.1/o-ran-ip-user-guide/o-ran-ip-functional-description
- zstd decoder memory tradeoffs: https://github.com/facebook/zstd/wiki/Using-libzstd-in-a-memory-constrained-environment
- UHD RFNoC packet/FPGA architecture: https://uhd.readthedocs.io/en/latest/page_rfnoc_fpga.html
