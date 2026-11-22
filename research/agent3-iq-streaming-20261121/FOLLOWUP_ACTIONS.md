# Agent 3 follow-up register — main session and local hardware

Status: to be validated on named targets. Prioritized by evidence value rather than sophistication.

## P0 — cross-project integration

- [ ] Locate research branch and SHA in `PUSH_PROVENANCE.md`; compare against current OrcStream main. Open reviewable research PR; do not merge speculative v1 wire semantics by default.
- [ ] Review all eight reports and new frontier experiments for sources, performance and licensing caveats.
- [ ] Define ADRs on sample format vs. codec, explicit epoch/context, authentication and max packet/decode sizes, negotiated mandatory minimum and independent source/transport adapters.
- [ ] Update OrcStream ROADMAP / STATUS / project truth with new *proposed* gates only when reviewed. Separate source hardware work into OrcPluto-Firmware.
- [ ] Preserve experimental C99 v0.1 as unmerged and version-incompatible until an explicit migration review.

## P1 — data / test infrastructure

- [ ] Build reproducible capture-corpus manifest: 2+ SDR vendors; true IQ raw format, known tuning / gain / RF inputs; phase-coherent dual channel if available; signed 12-bit range check where applicable; hashes, timestamps/clock domain, redistribution rights.
- [ ] Add clear controls for incompressible noise, Gaussian noise, bursts, CW, AM/FM, PSK/QAM, OFDM, LoRa chirps, packet frames, near-far weak+strong, packet loss, OOD waveforms.
- [ ] Benchmark raw16, exact pack12, raw bypass, delta + Rice, LZ4, zstd, BFP8/10/12 at exactly matched inputs and fidelity contracts.
- [ ] Set hard max working memory, encoded expansion, p99 encoder/decoder deadline, packet loss/epoch behavior and failure policy.
- [ ] Run each experiment from a working directory and save logs in that same directory.

## P2 — local Fishball work (requires real board/PC)

- [ ] Record exact Zynq-7020 / AD9361 board rev, shipped image/FPGA, IIO scan metadata / channel order, interfaces and safe rollback image; do NOT assume board family identity proves feature availability.
- [ ] Native bit-preserving capture: verify sign extension / endian / signed12 range at multiple sample rates and channels; hash each chunk and reconstruct exactly from pack12.
- [ ] Throughput and overrun: measure producer sample counts, DMA drops, measured USB bus speed, transport goodput and reconnect; retain stock IIOD coexistence.
- [ ] Add the bounded FPGA block packer only when simulation and Linux producer have passed; document AXI DMA and timing closure tests.

## P3 — potential advanced features

- [ ] Compare progressive lower-precision IQ base + residual exact layer against separate plain streams. Validate exact reconstruction or fail explicitly when an enhancement frame is unavailable.
- [ ] Evaluate FPGA event / subband / triggered capture with explicit sample windows and detection ROC, never masquerading as full-band raw IQ.
- [ ] Evaluate dual-channel cross-prediction and phase-coherence, plus unsynchronized clocks as negative control.
- [ ] Consider learned selector and neural encoder *only* when practical integer baselines, independent decoders and corpus discipline exist.

## Main-session output contract

Report accepted/rejected findings, published code and SHA/branch, edits/PRs per repository, unresolved decisions, actual measured outcomes with sample corpus and log path, and 3 highest-value next tests. Separate RESEARCH / PROPOSED / IMPLEMENTED / MEASURED / BLOCKED in every release claim.
