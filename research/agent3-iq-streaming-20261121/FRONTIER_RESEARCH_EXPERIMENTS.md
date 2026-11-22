# Frontier experiments to differentiate OrcStream

Status: research proposals, **not** implemented features or demonstrated performance. 2026-11-21. Owner priority: earn a widely adoptable, hardware-independent SDR wire + SDK through reproducible signal-quality evidence, not compression marketing.

## Priority bets (ordered)

### F1 — Progressive IQ with verifiable exact-recovery residual

Transmit a quickly decodable lower-precision base layer, followed by one or more optional reconstruction residual bitplanes. When **all** exact residual layers are received, the original declared integer samples must reconstruct bit-for-bit; base alone is explicitly lossy. The receiver may request the residual from a bounded source cache, or subscribe only to a base quality profile.

**Test:** compare exact signed12 bitpacking, BFP8/10 base + residual, MSB bitplanes + entropy-coded residual, and raw CS16. Sweep block sizes, signal power imbalance, RF chirps, CW, bursts and random noise. Inject 0.1–10% packet loss with delayed/reordered enhancement data. Track transmitted bits *including* metadata/retransmissions; end-to-end latency, layer deadline, peak RAM, exactly reproduced file SHA, EVM/PSD/BER before residual, and recovery behavior on missing residual. Use immutable sample indices and epochs across layers.

**Possible advantage:** unmatched flexible clients (tiny MCU gets a declared low-bandwidth view; desktop may reconstruct exact when all layers arrive). **Failure condition:** layer overhead/CPU cost beats plain parallel profiles or impossible memory footprint on Tier 0.

### F2 — Truthful spectrum-aware multi-product streamer

FPGA DSP detects occupancy, optionally channelizes the RF span, and sends narrow channel IQ, spectrograms, events and triggered raw windows as **separately named derived products**. An opt-in client configures the event false-alarm/sensitivity budget and can retain complete raw capture elsewhere. Do not label these products compressed raw wideband IQ.

**Test:** inject rare signals across changing occupied bandwidth using known RF generators and prerecorded multisignal captures, weak signal near threshold and bursts shorter than FFT dwell. Measure event misses, Pd at fixed Pfa, captured pretrigger window completeness, blind spots, transform provenance, total useful delivered receiver bandwidth per link bit, and raw comparison. Compare against a fixed always-on wideband IQ stream under identical USB/network limits.

**Possible advantage:** usable 50+ MHz *observation* on constrained links through metadata/derived channels where continuous full-band raw IQ cannot fit; no claim of reconstructing full-band original. **Failure:** unreported missed emissions, no transparent sample window, or insufficient on-device resources.

### F3 — Weak-signal guardrail / adversarial distortion suite

A codec that looks good under average EVM may destroy faint signals beneath a loud interferer. Make OrcStream's differentiator that every lossy claim includes calibrated weak-target detection and strong/weak near-far tests.

**Test:** use paired identical IQ corpus with weak CW/FSK/chirp/short packet at -20 to +10 dB SNR, powerful adjacent blocker, impulsive interference, ADC clipping and moving carrier. Evaluate ROC Pd at fixed Pfa, demodulator BER/PER (with confidence intervals), noise-floor PSD difference, spectral spur changes and synchronization across phase-coherent channels. Fix blind evaluation sets and compare BFP8/10/12, truncation, μ-law, neural, bitplane strategies at **equal effective rate**. **Failure:** greater compression at cost of unacceptable detection loss or an unbounded error mode falsely declared bounded.

### F4 — Small adaptive selector trained offline, integer-only at runtime

Train an offline per-block policy using inexpensive statistics such as block peak, quantizer occupancy, differencing histogram, zero run, simple entropy estimate and spectral occupancy; compile the selected policy to a tiny deterministic decision tree or table. Candidate codecs: raw, packed12, delta+Rice, simple predictor, BFP (opt-in) and bounded LZ4. Distinguish per-fidelity-mode selectors.

**Test:** pre-register train/validation/test capture splits by receiver and environment (no same-capture leakage), compare oracle best choice, fixed codec baselines, heuristic selector and tiny learned selector. Include overhead and worst case; track selector CPU, dynamic working memory, p99 encode time, dropped samples, bytes transmitted at equal fidelity, MCU/ARM decode throughput. **Failure:** overfit to synthetic scenes or greater end-to-end loss than a fixed simple codec.

### F5 — Phase-aware spatial redundancy for coherent dual RX

Investigate cross-channel predictors, complex decorrelation and common-mode rejection for synchronized dual receivers sharing an LO. This must *not* assume separate receiver channels share exact amplitude/phase or calibrate away arbitrary channel phase. A lossless predictor transmits exact residuals, and lossy mode declares impact on AoA/phase.

**Test:** coherent splitter/known phase delay, IQ mismatch, antenna baselines, phase drift, independent receivers with unsynchronized clocks as adversarial control. Track exact hash, inter-channel phase/lag errors, direction-of-arrival bias, packet-loss gaps, and compressed rate compared with independent-channel coding. **Failure:** synchronization assumptions do not hold or overhead exceeds correlation savings.

### F6 — FPGA-native progressive packer + ARM controller

Create a streaming 12-bit packer and integer BFP block codec with bounded block latency in Zynq PL; keep negotiated protocol/session/codec profile logic on ARM. Do not put the network protocol into HDL by default.

**Test:** golden vector equivalence C99 vs RTL/Verilator, all signed corner cases, random backpressure, AXI ready/valid violations, DMA overruns, p99 time to on-wire frame, LUT/FF/BRAM/DSP, power/thermals, throughput versus PS-only implementation; preserve stock libiio in a concurrent client. **Failure:** timing/resource closure or acquisition reliability regression.

### F7 — Task-aware (not task-only) neural IQ challenger

Try small causal convolutional/predictive or lightweight quantized neural methods *offline* on CUDA/workstation, then compare with integer predictors/transform codecs on identical RF samples. Train for EVM plus weak signal/phase/BER penalties; evaluate raw IQ only under actual measured sample/task fidelity. A latent embedding or modulation class is not reconstructible IQ.

**Test:** cross-radio/cross-location holdouts, OOD waveforms and jammers, real recordings, tiny burst boundaries, multi-channel phase, decoder-only CPU budget, energy cost, deterministic failure, worst-case bytes and exact residual fallback. **Failure:** no clear Pareto win (rate + reconstruction + latency) over BFP + predictors; learned models too costly for the small endpoints.

### F8 — DIFI/VITA interoperability as measurable advantage

Ship independent C99-compatible producer/consumer and a standards bridge with packet captures and third-party fixtures. Explicitly map context IDs, timestamps and gaps; publish incompatibilities. DIFI 1.3.0 is a current reference standard but OrcStream does not assert it is binary compatible.

**Test:** conformant captures and loopback in 2 independently written implementations; Wireshark dissector, packet jitter, byte-order fuzz, unknown TLV and malicious frame tests. **Failure:** mislabeled timing/clock provenance or integration requiring OrcSDR/Pluto internals.

## Why these are grounded research directions

- O-RAN standardizes BFP/other IQ compression in a cellular fronthaul context, but arbitrary wideband SDR time-domain validity is a separate experiment: https://www.etsi.org/deliver/etsi_ts/103800_103899/103859/12.00.01_60/ts_103859v120001p.pdf
- An FPGA+GPU spectrum-monitoring study (2026) reports event/duty-cycle data reduction on five SDRs; that reduction is **not** lossless wideband IQ compression: https://pmc.ncbi.nlm.nih.gov/articles/PMC13417223/
- A 2025/2026 O-RAN BFP9 evaluation reports link-performance sensitivity at higher modulation/coding rates; aggregate EVM alone is not enough: https://www.politesi.polimi.it/handle/10589/243969
- Recent RF foundation models show that learning can support modulation/angle-of-arrival tasks, but classification embeddings do not prove IQ reconstruction: https://arxiv.org/abs/2506.06718 and https://arxiv.org/abs/2611.03947
- Recent RF generative augmentation study explicitly notes that downstream task utility need not track spectral fidelity: https://www.mdpi.com/2227-7390/14/17/3131
- DIFI 1.3.0 interoperability specification: https://dificonsortium.org/standards/

## Study design discipline

Pre-register hardware identity, image/FPGA hashes, source/native layout, RF calibration, fixed datasets and train/test split, sample rates and channels; report both native bits/sample and padded-container comparison, encoder/decoder rate, CPU cycles, p99, memory, power if measured, transport goodput and packet losses. Use paired confidence intervals across captures, not cherry-picked best examples. Save log files next to scripts/capture manifests. Do not report synthetic figures as actual radio performance. Any V1 spec change needs test-vector versioning and independent decode.
