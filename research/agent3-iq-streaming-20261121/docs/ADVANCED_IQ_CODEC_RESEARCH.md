# Advanced IQ Codec Research — OrcStream Agent 3

**Status:** RESEARCH / design hypotheses · **Date:** 2026-11-21 · All numerical results must be tied to a corpus and version; no hardware-compression performance is claimed.

## Mathematical boundaries and measurement units

For complex sample rate `Fs`, channels `C`, and effective bits per I and Q `b`, the **native information-container rate before coding** is `R_native = Fs × C × 2b` bits/s. A CS16 container uses `32 × Fs × C` bits/s; this can exceed the source's nominal bit resolution. For 61.44 MSPS, C=1: native 12+12 bits = **1.47456 Gbit/s**, CS16 = **1.96608 Gbit/s**; C=2 doubles. USB 2.0 High-Speed's 480 Mbit/s is a PHY signaling ceiling, not a sustained user-data rate. A hypothetical 320 Mbit/s user-link budget would require `24/ (320e6/61.44e6) = 4.608×` compression *from native 24-bit complex IQ* (and `1.96608/.320=6.144×` from padded CS16); the budget is an illustration, **not** a measured Pluto link. Such ratios are not guaranteed on random IQ.

**Shannon/information theory:** A lossless code cannot uniformly shorten every possible `n`-bit string. If source entropy approaches 24 bits/complex sample, a stream comprising independent, uniformly distributed signed-12 I/Q components is essentially incompressible after reversible packing. A properly labeled output mode with reduced sample rate, spectrum-only features or omitted noise-floor energy is a different information product.

**Metrics:** `bpc`=bits per input complex sample after coding including codec-side payload metadata; `ratio_native = 24/bpc` for a 12-bit I/Q capture; `ratio_padded = 32/bpc` for CS16 container; include packet headers separately; report encoder/decoder MB/s, cycle/sample, peak RSS/heap, per-block expansion, p50/p99 delay, EVM/PSD/BER/PER and cross-channel phase effects. Specify whether compression mode is negotiated per epoch or per block.

## Ranking (recommendation, not measured performance)

| Priority | Method | Fidelity | Why | Risks / device class |
|---|---|---|---|---|
| P0 | Validated CS12 bit packing | Bit-exact when all values fit | 25% reduction vs 16-bit padded without entropy assumptions | Prove source bit-depth and sign extension; Tier 0–4 |
| P0 | Identity + bypass selector | Bit-exact | No expansion in high-entropy scenes except explicit framing | Per-block decision and decoder state; all tiers |
| P1 | BFP integer block codec | Bounded error after precise definition | Deterministic, low memory, FPGA-friendly | Weak-signal/phase/coherence risk; Tier 1–4 |
| P1 | Predictor + Rice on exact residual | Bit-exact | Natural for correlated samples; bounded integer arithmetic | Can be worse than raw on noise; Tier 1–4 |
| P1 | zstd / LZ4 on packed data | Bit-exact | Mature fast baseline, good on low-entropy scenes | Work RAM, variable runtime; Tier 2–4 |
| P2 | Scale/compand with explicit error | Lossy, measured | Dynamic-range adaptation | Quantization tail, calibration changes; Tier 1–4 |
| P2 | Time-domain transform/channel-aware | Lossy or derived | Good for selectively useful subbands | Not equivalent to original IQ; high CPU/FPGA |
| R&D | ANS/rANS, hybrid entropy backends | Bit-exact or combined with lossy front end | Better residual coding if enough skew | Lookup/model complexity; Tier 2–4 |
| R&D | Learned codec | Usually lossy | Potential structured-signal gains | Compute, model provenance, distribution shift; not v1 mandatory |

## Detailed algorithm-family comparison (16 approaches)

Each entry specifies *representation*, *objective*, *cost/latency*, *failure modes* and *hardware suitability*. Proposed thresholds are experiments, not observed outcomes.

### 1. Reversible container reduction / bit packing

Input: measured, sign-extended `CS16_LE` whose every I/Q value is in [-2048,2047]. Encode low 12 bits I followed by low 12 bits Q, two's complement, e.g. **3 bytes per complex sample**. Decoder sign-extends to the original 16-bit values. A random signed-12 stream still yields exactly 25% fewer bytes vs 16-bit padded. Costs: shifts, masks and a small output FIFO; reversible independent frames; a strong Tier 0/FPGA primitive. **Reject** if actual range or sign extension cannot be proven. This is not Shannon compression; distinguish `valid_bits=12` from container bits [R1].

### 2. Sample-planar order / byte shuffle / bitshuffle

Change order (I-plane Q-plane, low-byte/high-byte, bit planes), without losing bits, to cluster correlated symbols for a secondary compressor. Independent transform can be inverted with bounded block memory. Cost `O(N)` plus a buffer or DMA layout change; often beneficial for sign-extended formats, may degrade contiguous IQ/predictor locality. FPGA should favor streaming-compatible transposes with small line/block buffers. Experiment paired vs planar with same downstream zstd/LZ4 and *same baseline samples*.

### 3. Temporal delta coding

For signed `w`-bit samples, define reversible residual `r[n]=(x[n]-x[n-1]) mod 2^w` with known initial state; map via zigzag *only if exact range behavior is specified*. Noise with independent samples yields high-entropy residuals; smooth CW/FM can help, especially combined with Rice. State reset at frame boundary or explicit checkpoint. Encoder two subtractors, decoder adders; FPGA-/MCU-friendly. Block-local state avoids packet-loss propagation.

### 4. Adaptive linear prediction / LPC

Use integer predictor `p[n]=sum a_k*x[n-k]` on I/Q separately; encode exact residual and coefficients per independently decodable block. Adaptive coefficients may make CPU/FPGA cost and model metadata dominate. Benefit requires stationarity; noisy wideband and abrupt bursts can cause failures. Start with simple `p=x[n-1]` or `p=2x[n-1]-x[n-2]`, compare against order 2/4 learned adaptive, and cap coefficients/workspace.

### 5. IQ-coupled and inter-channel prediction

Exploit I/Q or coherent receiver cross-correlation only where measured. Predictor requires exact channel/sample alignment and preserves both channels when using lossless residual coding. Delay buffering and clock skew amplify risks; include reference channel/order, epoch and sample offsets. Phase-sensitive direction finding is a strong rejection test if implementation rewrites or reorders channels. Not a default for unrelated RX channels.

### 6. Rice/Golomb residual coding

Map signed residual to unsigned, quotient unary + remainder bits. Small hardware divider-less shift/bit primitives for power-of-two Rice parameter `k`; per-block parameter estimated from residual magnitude distribution. On high-entropy inputs, use a bounded unary escape or raw fallback, since uncapped unary may explode. Very attractive for portable C99/RTL. Compute bytes/sample and worst-case length analytically for each `k`, avoid treating a Rice stream as self-delimiting absent sample count.

### 7. Huffman/static entropy coding

Canonical code lengths or fixed model for residuals; exact when framed. Decoder tables and code-length metadata cost RAM, but no arithmetic multiply/divide needed. Rebuild/ship model only if savings exceed header and complexity. Bad adaptation and small frames frequently lose to packed raw; MCU static model possible. Safety: cap codeword lengths, prohibit ambiguous trees, track bitstream EOF.

### 8. Arithmetic coding / ANS / rANS

Higher entropy efficiency with good symbol probabilities than simple Rice in some distributions; often multiple reverse-coded streams required for high-performance rANS. Decoder table memory and state flush/restart need exact versioned spec. High-speed host vectorization possible, FPGA integration substantially less trivial than BFP/Rice. Do not include as a v1 mandatory codec without reproducible advantage after framing and resource costs [R2].

### 9. LZ4 and DEFLATE

Lossless byte compression of packed CS12; inexpensive established baselines. LZ4 favors fast decode/low latency, DEFLATE has broad compatibility but can use significant CPU. When data is near random, can **expand** even if CS16 representation compresses due to redundant sign bits. Require per-frame `encoded_bytes < raw_bytes - decision_margin` or identity fallback. Embed exact framing, decoder window limit and source format. Not necessarily ideal for FPGA. Source projects [R3].

### 10. zstd on packed/native IQ

Strong general-purpose baseline with tunable speed and memory. Particularly useful on zeros, bursts, cyclic pattern but may barely compress high-entropy packed12. Compare -1/-3/--fast against LZ4 on identical inputs; report decoder RAM and scratch allocation. Favor Tier 2/3/4; never make it mandatory on Tier 0. A zstd stream's decode memory must be capped and vetted for hostile size claims [R4].

### 11. Block floating point (BFP)

For each block of `B` complex samples, choose integer exponent `e >= 0` sufficient for both I and Q signed mantissas of `m` bits; emit `2Bm` bits plus `e` metadata. Decoder `x_hat = mantissa << e`. With arithmetic right shift, `|x_hat-x| <= (2^e - 1)` in source LSB units; a rounding-to-nearest variant needs its own independently tested bound and overflow rules. With B=256, m=8, exponent=one byte, nominal **16.03125 bpc** (vs native 24), regardless of signal content, ignoring OrcStream frame overhead. Strong deterministic FPGA choice. However, a strong interferer can force a large exponent and erase nearby weak IQ precision. ETSI O-RAN BFP blocks are *frequency-domain PRBs of 12 subcarriers*, not arbitrary time-domain sample groups; generalization must be documented [R5].

### 12. Block scaling

Shared block scale multiplier and quantized mantissas; similar BFP intent but division/multiplication or LUT approximations add fixed-point complexity. O-RAN Annex A describes a common scale quantizer. Benefit may be better use of mantissa span than powers of two, but encoding and reconstruction error bounds need an exact rational scale and rounding specification. DSP slices multiply; table resource and worst-case pipeline are hardware cost [R5,R6].

### 13. μ-law / nonlinear companding

Nonuniform quantizer prioritizes lower amplitudes; can protect weak samples better in some distributions but distorts EVM, amplitude linearity, spectral products and calibration. O-RAN defines a block form; generic telephony μ-law byte mapping cannot be treated as RF-equivalent. Need explicit companding curve ID/parameters, monotonic lookup, saturations, reconstruction error tables and reference bit vectors. Tier 1+ if error policy permits [R5,R6].

### 14. Fixed-bit reduction / bounded quantization / noise shaping

Truncate/round 12→10/8 I/Q, with declared gain/scale and exact rounding. Size is predictable: 20 or 16 native bits/complex plus framing. **No universal absolute error** under automatic gain changes unless scale/exponent is included. Dither/noise shaping can improve application-specific spectral effects but changes raw samples and may hurt coherent phase or weak detectors. Fixed-point encode/decode is easy; analytic error bound possible if input range and quantizer are specified.

### 15. FFT/wavelet/subband transform with sparse coefficients

High value when energy concentrated in sparse bins; costly when noise floor occupies all bins. Complex FFT itself can be numerically lossy unless exact reversible integer lifting; coefficient dropping loses time-domain information, phase and fine noise. Requires window, overlap, transform normalization, bin positions and latency. A filtered/decimated **channel IQ** or FFT-only **SPECTRUM** should usually be declared as a derived product, rather than marketed as general IQ compression. FPGA channelizer can save bandwidth when only partial RF spectrum is requested [R7].

### 16. Learned prediction / neural autoencoders / hybrid policy

A learned predictor can feed exact residual entropy coding, or an autoencoder can deliver lossy reconstruction. Both require a pinned model hash, architecture, integer/floating-point definition, bounded decoder memory, out-of-distribution error handling and side-information costs. Hypothesis: learnable structure may help repeated narrowband protocols. Counter-hypothesis: mixed high-entropy receiver noise defeats it and small ARM/FPGA budgets are poorly matched. **R&D only** until it beats bitpacking+BFP+Rice+LZ4/zstd at *equal fidelity* on held-out capture families and independent hardware.

## Proposed hybrid selector

Each independently decodable block chooses among:

```text
source adapter -> validate native representation and epoch
          -> [exact path] packed-native -> optional Rice / LZ4 / zstd
          -> [bounded-error path] BFP8/BFP10 / scalar quantizer
          -> [derived products] channelizer / spectrum / events (separate stream types)
          -> calculate complete wire size and deadline/resource estimate
          -> use selected mode iff receiver advertised codec AND policy permits
          -> otherwise identity/raw, or explicit bandwidth error; NEVER silent downgrade
```

For a block of `N` complex native12 samples, compare `encoded_bit_count + codec_header_bits + frame_bits`. Selector requires a *known worst-case bound* or a deterministic CPU/time ceiling; if the candidate exceeds budget, bypass. Never waste latency trying multiple expensive codecs without a sampling model or capacity budget. Train any selector only on a disjoint held-out corpus; report worst-case regret vs an oracle and all decode/resource failures.

## Preliminary controlled synthetic probe — not a hardware result

`../experiments/codec_probe.py` generated 9 deterministic synthetic CS12 complex streams, each 131,072 samples, with seed 20261121. Results are reproducible in `../experiments/synthetic_results.csv` and **do not** establish real RF, AD9361, FPGA or high-throughput feasibility. Illustrative results include:

| Synthetic family | Packed12 bits/complex | zstd -1 bits/complex on packed12 | BFP8 (256-sample blocks) bits/complex | BFP8 measured vector EVM |
|---|---:|---:|---:|---:|
| independent uniform 12-bit | 24.00 | 24.001 | 16.031 | 0.745% |
| Gaussian receiver-like noise, sigma 360 | 24.00 | 23.11 | 16.031 | 2.337% |
| CW/chirp | 24.00 | 12.27 | 16.031 | 0.690% |
| QPSK OFDM synthetic | 24.00 | 24.00 | 16.031 | 1.149% |
| noisy tone | 24.00 | 24.00 | 16.031 | 1.267% |
| bursty sparse | 24.00 | 6.00 | 16.031 | 1.467% |

The BFP8 prototype uses **flooring arithmetic shift**, which biases residuals and is not proposed as a quality-optimized production implementation. All EVM here is **codec reconstruction EVM vs source quantized IQ**, not actual modulation EVM vs ideal symbols. For signal amplitudes approaching zero, relative EVM can be meaningless; use a reference/full-scale normalization and report absolute LSB error. No runtime comparison here: the exploratory script uses mixed Python and native binaries.

## Rejected shortcuts and unavoidable risks

- Blindly claiming `12 bit → 8 bit = 1.5× compression` neglects clipping, scaling, rounding, error, overhead and whether the source was *actually* 12-bit.
- Compressing only detected occupied spectrum is a **derived capture service**, not full-band exact IQ.
- Adaptive bit-depth changes without explicit per-block codec ID and quantizer metadata destroy interoperable decoding.
- Error-concealing packet loss by fabricating samples destroys coherent timing; gap must remain explicit.
- Arbitrary AI-generated mode labels or classifier confidence should never be presented as ground truth of RF identity.

## First research experiments

1. **Native packing exactness:** RTL-SDR CU8, Pluto CS16 and a second independent signed RF frontend, byte-level validation of range/endian, adversarial extrema and forced overflow; hash roundtrip.
2. **Lossless selector:** independently decodable 4 KiB and 32 KiB blocks: raw16 vs packed12 vs predictor/Rice vs LZ4 vs zstd; test high-entropy, mixed RF and silence, p99 latency and CPU/RAM; require zero silent size-expansion-induced drops.
3. **BFP fidelity:** m=8/10/12, B=16/64/256; emulate exact fixed-point reference and FPGA pipeline; weak signals underneath strong adjacent interferer, phase-correlation, BER/PER/EVM and PSD tests. Reject any mode failing declared error.

## Sources

[R1] Analog Devices AD9361 Linux driver, 12-bit 2's complement interface and sample-rate constraints: https://developer.analog.com/docs/linux/drivers/iio-transceiver/ad9361/

[R2] Zstandard repo and APIs (reference for ANS/FSE practical tradeoffs): https://github.com/facebook/zstd

[R3] LZ4: https://github.com/lz4/lz4 ; zlib: https://zlib.net/

[R4] zstd memory-constrained guidance: https://github.com/facebook/zstd/wiki/Using-libzstd-in-a-memory-constrained-environment

[R5] ETSI TS 103 859 V7.0.2, Annex A: https://www.etsi.org/deliver/etsi_ts/103800_103899/103859/07.00.02_60/ts_103859v070002p.pdf

[R6] Altera O-RAN compression IP: https://docs.altera.com/r/docs/683238/25.1.1/o-ran-ip-user-guide/o-ran-ip-functional-description

[R7] UHD RFNoC FPGA architecture: https://uhd.readthedocs.io/en/latest/page_rfnoc_fpga.html

## Research additions: six particularly instructive academic directions

1. **Adaptive LPC + entropy coding for real RF:** The 2023 context-aware FAPEC paper explicitly addresses interleaved signed16 SDR IQ, chooses LPC coefficients via autocorrelation/Levinson–Durbin and optionally selects quantization from signal detection. This is a better directly relevant baseline for OrcStream than assuming audio predictors generalize perfectly. Note that its typical independent chunk sizes of 64 KiB–8 MiB can be incompatible with low-memory MCU/p99-latency targets; test deliberately smaller windows. Source: https://pmc.ncbi.nlm.nih.gov/articles/PMC10098950/
2. **Channelized wideband + mixture quantization:** A 2026 Virginia Tech thesis evaluates channelization, occupancy detection and a two-term-Gaussian mixture quantizer, reporting better MSE at comparable compression than its tested adaptive vector-quantizer baseline. OrcStream should evaluate it as **Mode D channelized product** or Mode C only when full-band sample mapping is defined—not equivalent native IQ. Source: https://vtechworks.lib.vt.edu/items/475fdff7-c942-4a84-ae3d-c6205a510c60
3. **Learned compression for RF classification:** HQARF (2024) uses hierarchical vector quantization to preserve modulation recognition utility on a six-class task while describing phase reconstruction degradation at deeper compression stages. This argues for reporting classification utility *alongside* RF fidelity rather than replacing fidelity with classifier accuracy. Source: https://arxiv.org/abs/2403.03150
4. **Learned FPGA nonlinear quantizer:** Hisano & Yagi (2025) show a neural-quantizer hardware feasibility path with performance comparable to an optimized nonlinear baseline on their fronthaul problem. It is not proof a large neural encoder is necessary, and suggests a smaller **learned LUT** might be tested against classic μ-law/block scale. Source: https://doi.org/10.23919/comex.2025XBL0016
5. **Delta-Rice analog detector compression:** ORNL's 2024 work demonstrates predictor/decorrelator plus Rice and discusses FPGA/GPU implementation. Its reported detector-data throughput/ratio must not be advertised as IQ capability; use the **algorithm and benchmarking method**, rerun on real CS12. Source: https://www.ornl.gov/publication/delta-rice-hdf5-compression-plugin-optimized-digitized-detector-data
6. **Application-level error can be nonlinear:** Ahmed's 2025 fronthaul study reports BFP9 penalties much larger for demanding high-MCS than robust low-MCS modes on its evaluated setup. This strongly supports independently testing BER/PER and error thresholds, even when aggregate EVM looks small. Source: https://www.politesi.polimi.it/handle/10589/243969

**Research interpretation:** The literature reinforces a **portfolio approach**: representation packing + exact predictors for Mode A; bounded fixed-point quantizers for Mode B; task-specific measurement envelopes for Mode C; explicit channelized/feature products for Mode D. No publication above demonstrates universal high-entropy lossless full-band IQ compression over USB 2.0 at the full AD9361 maximum.
