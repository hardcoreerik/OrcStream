# Main-session handoff prompt — OrcStream Agent 3 IQ Compression Research

You are the **OrcStream / OrcPluto-Firmware main-session integration director**. The owner has authorized reviewing and integrating **research findings and prioritized development plans**, not an automatic merge of an experimental wire format or a destructive flash of any SDR.

## Authoritative sources and repositories

- OrcStream: https://github.com/hardcoreerik/OrcStream
- OrcPluto-Firmware (hardware reference, **not** core protocol dependency): https://github.com/hardcoreerik/OrcPluto-Firmware
- OrcSDR: https://github.com/hardcoreerik/OrcSDR
- OrcStream baseline prior to Agent 3: `main` commit `66c3a294d36c2ad1b38243b9146952cabb5aa22f` and the **unmerged** C99 draft branch `chatgpt/protocol-c99-v0.1-20261104` at `27c134060631a2efb74da04ee760e49957474d96`. Recheck latest heads before editing anything.
- Agent 3 research was committed to **a dedicated research branch**, not main. See `PUSH_PROVENANCE.md` in this handoff for the exact ref and SHA, and read the research/ folder there.

## Mission

Integrate the research into the cross-project plan, prioritizing a portable, credible and standards-friendly streaming SDK; source-independent codec, transport and fidelity semantics; and a Pluto-compatible Zynq-7020/AD9361 **reference** firmware, without making Pluto mandatory for OrcStream.

First read OrcStream's `PROJECT_TRUTH.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `STATUS.md`, `docs/PROTOCOL_EVOLUTION.md`, `docs/VALIDATION.md`, plus all eight Agent 3 research reports. Read `FRONTIER_RESEARCH_EXPERIMENTS.md` and `FOLLOWUP_ACTIONS.md`. Do not replace the canonical project roadmap wholesale with research proposals; reconcile line by line and preserve existing evidence distinctions.

## Integration work, in order

1. **Inventory and evidence:** inspect repo branch heads, the experimental C99 64-byte wire draft, all new research files, generated experiments and limitations. Verify there are no actual real-radio throughput measurements represented as such. Flag any doubtful citations or unsupported numeric claims. Record what is merged, proposed and only synthetically demonstrated.
2. **Decision log:** create an explicit ADR/decision table for mandatory baseline sample encodings, sample/packet counters and epochs, clock-domain semantics, context/TLV bounds, sample format vs. codec IDs, decompressed size and anti-bomb checks, endianness, signed12 representation, integrity vs. authentication, transport and capability negotiation, independent decode, and unknown codec handling. Treat the 80-byte v1 draft as a **proposal**, not a final standard.
3. **Roadmap integration:** create focused PRs updating canonical OrcStream roadmap/status/architecture and validation documentation with research-gated milestones. Add a tracked research experiment program and success criteria; link to the research branch/PR, don't copy uncertain assertions as project truth. Assign ownership: core codec/protocol -> OrcStream, Linux/PL/IIOD coexistence -> OrcPluto-Firmware, user-facing optional client integration -> OrcSDR.
4. **Implementation starting point:** review/compare C99 v0.1 to a proposed new experimental version, preserve v0.1 fixtures and tests, build separate conformance/codec-development branch. Do not silently reinterpret old fields or merge experimental code without review.
5. **First proof:** obtain real IQ on a named SDR from source-native scan layout; prove whether exact signed12 packing is valid and run bit-exact reconstruction, deliberate incompressibility/overrun tests and multi-client timing/gap proof. Compare raw CS16, exact packed12, delta+Rice, LZ4, zstd and adaptive bypass on identical samples. Add weak-signal + strong interferer challenge before any lossy claim.
6. **Second proof:** build BFP8/10/12 software golden reference and independent decoder, verify deterministic max error and real RF application metric tolerance; only then consider AXI/FPGA implementation and resource/power tests on known Fishball image.
7. **Frontier experiments:** prioritize progressive refinement layers (base + lossless residual), context-adaptive subband delivery as an explicitly **derived** service, and coherent multi-channel preservation. Treat neural compression as a challenger that must beat integer baselines at the same data, quality and compute budget, not an assumed upgrade.
8. **Review constraints:** no fabricated perf, no implied raw-IQ equivalence for decimation/events, no source-dependent protocol semantics, no silent quality changes, no public unauthenticated configuration, no broken IIOD compatibility. Preserve recovery/known-good boot image, and route board work to the hardware-specific repo.

## Concrete deliverables

- Revised OrcStream `ROADMAP.md`, `STATUS.md` and affected architecture/validation docs **as a reviewable PR** (or a documented exact patch plan if not authorized to merge).
- A tracked `research/` registry with experiments, code/data manifests, reproducible procedures, provenance, owners, numerical acceptance/failure gates and invalidation conditions.
- ADRs describing codec registry, fidelity contracts, v1-vs-v0.1 wire compatibility, and standard adapters (VRT/DIFI as adapters, not invented binary compatibility).
- Issue/PR backlog decomposed by repository, safe dependencies and hardware prerequisites.
- Local handoff with runnable commands, logs saved in run folder, expected outcomes and failure cases.
- A brief report: accepted findings, rejected/modified assumptions, measured vs. speculative claims, changed repo files/SHAs, risks and next three practical experiments.

## Questions requiring explicit decisions

- Is `RAW_NATIVE` truly the original scan representation or a documented lossless mapped representation? Preserve both if useful.
- Which sample formats/codec profiles are **mandatory** for a Tier 0 decoder? How are clients prevented from unbounded decoder allocation?
- Are sample index and epoch enough for discontinuities in source-side decimation/channelization, or are parent sample windows mandatory?
- Can an encoder guarantee constant-time bounded work even when incompressible noise appears abruptly?
- How do we establish non-hallucinated quality for weak signals and phase-coherent dual-channel receivers?
- What are codec-specific failure, reset, and late enhancement-layer behaviors over lossy UDP?
- What is the credible adoption on-ramp for an independent Soapy/GNU Radio/client before optimization for the Fishball FPGA?

**Follow-up:** Ask for board access / local capture artifacts only when a real hardware proof specifically requires them. On hardware-only work, commit reproducible scripts and a prompt that specifies capture paths, build commands, expected logs and safe rollback. Do not claim the assistant remotely executed on a board it could not access.
