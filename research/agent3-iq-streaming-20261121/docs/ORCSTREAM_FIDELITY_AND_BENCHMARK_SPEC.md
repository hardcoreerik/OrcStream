# OrcStream Fidelity Contract and Reproducible Benchmark Specification

**Status:** proposed test standard · **Date:** 2026-11-21 · Synthetic exploratory evidence is included, but **no real AD9361, FPGA, USB throughput, power or third-party codec benchmark was performed**. This document defines what would count as proof.

## 1. The four named fidelity modes

**Mode A — `EXACT` bit-exact lossless.** Decoder produces identical **declared native sample representation**, ordered channels, and sample counts for every successfully received sample. For CS16, exact original 16-bit words; a native sign-extended 12-bit capture may be represented in packed 12 bits *only if the source adaptation proves that mapping*. For CF32, “exact” requires bit-pattern identity including signed zero/NaN bits where applicable. Gaps remain explicit—delivered samples may be exact while a stream is incomplete. A hardware ADC's analog quantization is upstream of this contract.

**Mode B — `BOUNDED_ERROR`.** Select an exact quantizer profile and **maximum component error** `E∞ = max_n(max(|I_hat[n]-I[n]|, |Q_hat[n]-Q[n]|))` in the declared source LSB units or a precisely defined physical scale. The bitstream declares error bound, rounding, scale/exponent interpretation, saturation/clipping and behavior on out-of-range input. `mean error` is not a substitute for max error. BFP shift-to-floor with exponent `e` guarantees `E∞ ≤ 2^e - 1` source LSB under correct signed arithmetic and no clipping; a codec carrying variable `e` may report a maximum observed/allowed exponent per frame. Verify algebraic bound and exhaustive short-domain fixtures.

**Mode C — `MEASUREMENT_QUALITY`.** An opt-in lossy profile with **specified workload and metrics**: codec reconstruction EVM (on actual source IQ), PSD bias, detection performance, demodulation BER/PER, phase/coherence drift and calibrated amplitude preservation. The profile **cannot** guarantee every unseen modulation/scene is equivalent merely by meeting a single EVM value. An explicit unsafe-for-coherent flag may be necessary for some quantizers. On threshold failure, report, change profile with consent or terminate; never silently relabel the stream.

**Mode D — `DERIVED_PRODUCT`.** Filtered/decimated channels, spectrum/waterfall, event/trigger windows, demodulated messages and inference features. They may serve the user better than transporting the full ADC stream, but are **not reconstructible full-band raw IQ**. Must carry parent source/epoch/sample window and the exact processing chain, filter response and timing group-delay metadata when applicable.

## 2. Canonical measurement definitions

Let `x[n] = I[n]+jQ[n]` be declared source samples and `y[n]` the reconstructed samples aligned by parent epoch and first-sample index. A packet gap is not an eligible value `y[n]` for accuracy scoring and must be reported separately.

| Metric | Normative formula / method | Gotchas |
|---|---|---|
| exactness | SHA-256 over ordered native sample words **and** component-wise comparison; `mismatched_samples=0` | Hash alone doesn't prove channel/time alignment; still check metadata and counts |
| max component error | `max(|Re(y-x)|, |Im(y-x)|)` | Source units, scale and saturation explicit |
| complex RMS error | `sqrt(mean(|y-x|^2))` | Report source full-scale and selected reference normalization |
| codec reconstruction EVM | `100*sqrt(sum(|y-x|^2)/sum(|x|^2))` | Not ideal-symbol modulation EVM; becomes unstable for near-zero x |
| reconstruction SQNR | `10 log10(sum(|x|^2)/sum(|y-x|^2))` dB | Infinite if exact; finite if denominator nonzero |
| PSD error | Welch on aligned source/reconstruction with same NFFT/window/overlap; report dB median, p95, max per active/quiet bin | PSD of very weak bins needs noise-floor treatment and confidence intervals |
| weak-signal detect | Pd at fixed false alarm Pfa across SNR sweep | Must use holdout signals; compare same upstream IQ |
| phase/coherence | `angle(y*conj(x))` error distribution; for dual channel compare cross-spectrum phase and time lag | Ignore zero-magnitude samples under declared mask; different clock ≠ codec artifact |
| demod impact | Symbol timing lock, BER/PER/CRC for modulation and known payload | Different decoders may have thresholds; use pinned decoder configuration |
| latency | capture/encode/send/receive/decode time with clock provenance; p50/p95/p99 | Cross-device clocks require verified synchronization; otherwise local stage timestamps only |
| packet/continuity | loss, out-of-order, duplicate, source overruns, subscriber drops by separate counter/epoch | UDP sequence loss cannot unambiguously imply ADC loss |
| compression ratio | `native_input_bits / (encoded_payload_bits + codec parameter bits)`; separately report padded-container and **wire total** | Always compare 24-bit native12 vs 32-bit CS16 separately |
| worst-case size | max output bytes / input bytes over adversarial blocks and analytic codec bound | Bound allocations before decoding; raw fallback when worthwhile |
| throughput | end-to-end delivered valid complex samples/s; CPU process time, RAM peak, FPGA DSP/BRAM/LUT | A capture that drops upstream doesn't earn its configured rate |

**Fidelity acceptance must include weak-signal and direction-finding tests.** A visually similar waterfall is insufficient proof of equivalent IQ.

## 3. Reproducible corpora

### Corpus inventory and minimum provenance

| Class | Synthetic or hardware source | Required perturbations / purpose |
|---|---|---|
| Uniform independent signed12 noise | seeded deterministic synthesizer | adversarial entropy, nominal incompressibility |
| Receiver noise | synthetic quantized Gaussian and **actual RF front-end** with gain settings | stress entropy, clipping, actual ADC distribution |
| Quiet / constant / small amplitude | synthetic and real terminated input | discover redundancy but do not claim RF-general ratio |
| CW, chirp, AM, FM | synthetic reference + hardware injected known signal | phase, predictable structure, frequency offset |
| BPSK, QPSK, 16/64-QAM, FSK | pinned synthetic waveform + over-air/injected captures | codec EVM and demod BER/PER across SNR |
| OFDM / multi-carrier | seeded symbol mapping, random payload, known pilots and guard | noise-like waveform and fidelity under high PAPR |
| Weak + strong adjacent transmitter | synthetic paired signals then RF combiner | BFP weak-signal loss and adjacent leakage |
| Dense wideband multi-signal | actual legal RX captures with source/gain/channel metadata | heavy occupancy and unforeseen structure |
| Burst / intermittent | seeded burst windows and real transmitter captures | adaptive block behavior and event positions |
| Coherent dual RX | calibrated shared-clock/channels with phase-shifted injection | inter-channel phase, alignment, DOA impact |
| Pathological alternating/extreme | seeded full-scale extrema / signed-range edge cases | predictor expansion, overflow/shift correctness |
| Float specials | CF32 bits for NaN/signed zero/subnormal/infinity | Mode A bit-pattern guarantee |
| Packet faults | drop/dup/reorder/bitflip/partial header/hostile declared length | receiver safety and accurate gap accounting |

**Real capture fixture manifest** (per file): source UUID pseudonym, vendor/model/hardware revision, actual recorded center Hz, actual readback sample rate rational, IF/analog bandwidth, gain/manual AGC, temperatures and clocks if available, channel count/shared clock, exact sample/container formats and signed valid bits, scan-mask/channel order, original SHA-256, duration/count, capture timestamp validity, overrun state, consent/licensing to redistribute. Compress and license capture fixtures separately from source code.

### Split to prevent bias

- `train` / `selector_tune` (if algorithm needs adjustment): source devices and RF scenes disjoint from `holdout` where practical.
- `holdout`: at least two SDR hardware families and two receiver configurations. Hypothesized fingerprint/codec advantages do not count if measured only on training fixtures.
- `adversarial`: fuzz, max/min, clipped and nonstationary patterns, packet loss; never cherry-pick only compressible quiet captures.
- Performance conditions: 1/2/4/16/64 MSPS (synthetic target rates—hardware may not support), 1 and 2 RX channels, sample containers CU8/CS16/CF32, blocks 256, 1,024, 4,096, 16,384 complex samples as devices support; approximate if bandwidth and buffer limits require.

## 4. Benchmark pipeline and fairness rules

1. Fix source capture bytes and manifest; run source integrity check and record exact hash and sample count.
2. Convert to declared input format **once**; if testing CS12 packing, verify each component fits signed 12-bit and that container layout is specified.
3. Per test, encode the *same* sample range and count with identity raw16, raw packed12 (when exact), LZ4, zstd, Rice/predictive, BFP and other candidates; do not alter gain, decimate or squelch one contender without recording a different product/fidelity mode.
4. Include codec parameters, block headers, padding, frame and transport overhead in distinct numeric columns. Require decoder complete payload roundtrip or Mode B/C numerical metrics and error bound validation.
5. Run per-method warmup and at least 30 timed intervals for stable benchmarks; pin host CPU governor, ISA, process affinity where possible, input data in-memory, compiler flags, library versions, and block-count schedule. No debug vs release comparison.
6. Measure separately: source capture; encoder; packaging; transmit; receiver parsing; decoding; delivered-sample throughput; bounded queues, p95/p99 and loss.
7. Repeat with fixed network budgets and jitter/fault injection. A codec may compress well offline but fail real-time deadline (and is then not useful for that tier).
8. Re-run lossless hash and metadata/epoch/gap tests after packet fragmentation, packet reorder, codec state reset and reconnection.
9. Include all failed cases, expansions, crashes, missing caps, rejected fidelity modes and resource budgets in report.
10. Compare to competitor releases or pinned GitHub commits with equal native samples and **equal quality**, including SDR++/SDRangel/IIOD; if an external codec requires prior quantization, display the quality change.

### Report CSV schema (minimum)

`utc,repository_commit,codec_id,codec_version,implementation_backend,host_arch,cpu_model,os,compiler_flags,firmware_build,fpga_hash,device_revision,source_id,fixture_sha256,source_format,native_valid_bits,channels,sample_rate_hz,block_samples,source_samples,raw_native_bytes,raw_container_bytes,encoded_payload_bytes,codec_metadata_bytes,wire_bytes,lossless,peak_lsb_error,complex_rms_error,codec_evm_pct,psd_p95_db,ber_delta,per_delta,weak_pd_delta,source_drops,network_drops,subscriber_drops,encoder_MBps,decoder_MBps,cpu_pct,peak_rss_bytes,decoder_scratch_bytes,fpga_lut,fpga_bram,fpga_dsp,first_sample_latency_ms,p50_ms,p95_ms,p99_ms,power_w,notes`

Use `NA` for unmeasured fields, **never zero**. Publish scripts, manifests, standard logs and commands. The benchmark suite is allowed to determine that a new technique performs worse than packed raw IQ.

## 5. Portable test gates

| Gate | Minimum release criterion |
|---|---|
| Mode A | 0 mismatched source sample words across advertised format vectors, channel order, epochs and endian; no silent loss |
| Mode B | mathematically documented per-component `E∞` checked exhaustive small-domain and randomized high-range tests; packet declares bound |
| Mode C | pre-registered profile goals (EVM, PSD, BER/PER, Pd/Pfa, phase) on disjoint real corpus; *no universal default pass number yet* |
| Parser | rejected malformed lengths/unknown critical fields, bounded resources, sanitizer/fuzz passes and no OOB allocation |
| Multi-source | two independent hardware families; same receiver and wire; no Pluto-only dependency |
| Portability | two independent decoders produce identical golden vectors; one low-resource build demonstrated |
| UDP/network | no hidden sample-filling; outage/change captured with sequence + sample gap/error; bounded worst-case backlog |
| FPGA | exact hardware-vs-reference bit vectors or bounded-error equality; recorded FPGA timing closure and resources |
| IIO coexistence | normal libiio operations and rollback remain functional when optional native OrcStream service is installed |

**Performance thresholds should be set with actual platform measurements** before acceptance. A proposed “2× compression” requirement would be invalid for uniform signed12 noise in Mode A. Do not set arbitrary universal EVM thresholds across all radio applications.

## 6. Exploratory synthetic probe generated in sandbox

The accompanying `../experiments/codec_probe.py` uses NumPy-generated signed12 complex samples with seed `20261121`; it produces `synthetic_results.csv` and `run.log` **beside the script**, regardless of working directory. Requires Python 3, NumPy; optional `lz4` Python module and `zstd` executable. The current environment had Python 3.13.5, NumPy 2.3.5, `lz4.frame`, zstd CLI. Nine 131,072-complex-sample inputs were measured for **encoded byte size** only (524,288-byte CS16 container and 393,216-byte reversible CS12 payload per input).

| Corpus | Packed CS12 bpc | zstd -1 packed bpc | BFP8 bpc | BFP8 reconstruction EVM |
|---|---:|---:|---:|---:|
| uniform signed12 random | 24.000 | 24.001 | 16.031 | 0.745% |
| quantized Gaussian noise | 24.000 | 23.11 | 16.031 | 2.337% |
| chirped tone | 24.000 | 12.27 | 16.031 | 0.690% |
| synthetic OFDM | 24.000 | 24.00 | 16.031 | 1.149% |
| sparse burst | 24.000 | 6.00 | 16.031 | 1.467% |

BFP here packs 8-bit I/Q and 1 exponent byte per 256 complex samples, with a **round-toward-negative-infinity** right shift. The EVM shown compares reconstruction to the generated CS12 IQ, **not** demodulator EVM against ideal symbols. There are no benchmarked per-second rates: mixing Python operations and native utilities would make a speed claim misleading. There is **no** signal injection, physical radio, PS/PL measurement, 61.44 MSPS acquisition or USB data link in this probe. Treat its size results as unit-level exploratory validation, not a cross-vendor performance study.

### How to run the synthetic probe

```bash
python -m pip install numpy lz4  # lz4 optional; zstd CLI optional
python experiments/codec_probe.py
# Check experiments/synthetic_results.csv and experiments/run.log
# stdout also available in experiments/session_stdout.log for this run
```

For Windows where `zstd` CLI is not installed, the script automatically skips its case; record the environment and do not compare unmatched codec sets. The script verifies packed12/CS16 bitwise IQ roundtrip internally. Its compression byte counts omit OrcStream frame overhead and depend on zstd/lz4 implementation versions. A later version should use in-process native codecs with pinned versions for fair timing.

## 7. Reference sources

- ETSI O-RAN Annex A math: https://www.etsi.org/deliver/etsi_ts/103800_103899/103859/07.00.02_60/ts_103859v070002p.pdf
- MathWorks independent O-RAN algorithm: https://www.mathworks.com/help/5g/ref/nroranblockcompress.html
- AD9361 maximum baseband rate and 12-bit interface: https://developer.analog.com/docs/linux/drivers/iio-transceiver/ad9361/
- UHD timing/transport principles: https://uhd.readthedocs.io/en/latest/page_rfnoc_fpga.html
- zstd tests and memory: https://github.com/facebook/zstd ; https://github.com/facebook/zstd/wiki/Using-libzstd-in-a-memory-constrained-environment
- Baseline OrcStream validation proposal: https://github.com/hardcoreerik/OrcStream/blob/66c3a294d36c2ad1b38243b9146952cabb5aa22f/docs/VALIDATION.md
