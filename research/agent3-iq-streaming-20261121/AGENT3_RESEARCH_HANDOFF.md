# OrcStream Agent 3 — Sandbox Research Handoff

**Date:** 2026-11-21

**Scope:** Research and design only; **no push, commit, branch creation, protected-branch changes, firmware modification, device access or live on-device test was performed.**

## Repositories inspected

- `hardcoreerik/OrcStream`, `main` **66c3a294d36c2ad1b38243b9146952cabb5aa22f**; eight existing planning documents read. Current `main` is architecture/research, not a shipped codec.
- `hardcoreerik/OrcStream`, unmerged `chatgpt/protocol-c99-v0.1-20261104` **27c134060631a2efb74da04ee760e49957474d96**; read wire draft, public C99 API and handoff. Has an experimental 64-byte header, sample formats, CRC, indices, no negotiated codecs or sessions. Its historical test claims were **read, not rerun here**.
- `hardcoreerik/OrcPluto-Firmware`, `main`: confirmed **empty** at inspection time; no file contents or code were attributed to it.
- `hardcoreerik/OrcSDR`: repository confirmed; no detailed codec code inspection in this research run.

## Deliverables

All eight originally requested files are in `docs/`:

1. `IQ_COMPRESSION_COMPETITIVE_LANDSCAPE.md` — standards, vendor software, academic/FPGAs and realistic evidence gaps
2. `ADVANCED_IQ_CODEC_RESEARCH.md` — 16 codec families, costs/fidelity and proposed hybrid selector
3. `ORCSTREAM_PROTOCOL_ARCHITECTURE.md` — layer separation, optional hardware and time/gap/security requirements
4. `ORCSTREAM_CODEC_PORTABILITY_MATRIX.md` — Tier 0–4 codec and bounded-memory recommendations
5. `ORCSTREAM_FIDELITY_AND_BENCHMARK_SPEC.md` — Mode A/B/C/D metrics and reproducibility requirements
6. `ORCSTREAM_COMPETITIVE_DIFFERENTIATION.md` — adoptability and falsifiable advantage claims
7. `ORCSTREAM_ENGINEERING_ROADMAP.md` — prioritized stages/exit gates, answers 10 mandatory research questions
8. `ORCSTREAM_V1_PROTOCOL_PROPOSAL.md` — **non-normative** 80-byte draft, exact offsets, codec IDs, TLV control and undecided fields

Experiments are in `experiments/`:

- `codec_probe.py` — 9 reproducible **synthetic** CS12 IQ cases; identity, 12-bit exact pack, zlib, optional LZ4/zstd, fixed-point BFP8 byte size and reconstruction metrics. Writes `synthetic_results.csv` and `run.log` next to script.
- `v1_draft_wire_vectors.py` — self-contained tentative wire fixtures: exact CS16 raw and signed CS12 packed, 3 samples, CRC32; writes `proposed_v1_wire_vectors.json` and `v1_wire_run.log`.
- `validate_bundle.py` — independent standard-library checks for eight docs, CSV expected sizes and wire-vector roundtrip/CRC; writes `bundle_validation.log`.
- `session_stdout.log`, `run.log`, `v1_wire_run.log`, `bundle_validation.log` — source evidence for these **sandbox synthetic** exercises.

## Most useful conclusions

1. Exact 12-bit packing yields **25% byte reduction relative to padded 16+16 input** if real source IIO samples are confirmed signed12 and sign-extended correctly. It yields **no entropy savings versus already packed native24**.
2. Uniform high-entropy 12-bit I/Q is incompressible losslessly to any substantial degree; preliminary synthetic zstd result ~24 bpc is consistent with the mathematical limit.
3. Standard O-RAN BFP is attractive to FPGA pipelines but defined in PRB/frequency-domain context; adapting to arbitrary time-domain samples is **new codec design**, not blanket standards compliance.
4. Published RF research (FAPEC+LPC, Delta-Rice, learned VQ, channelized wideband mixture quantization) supports testing several codec families, but no paper licenses a universal compression ratio or fidelity claim.
5. A successful vendor-neutral SDK will need exact input representation + explicit fidelity policies + clock/epoch/sample gaps + independent codec registry + strict decoder scratch bounds + non-Pluto adoption.
6. The v1 proposal is intentionally **unfinished** in certain mandatory TLV layouts. Do not freeze, ship, or declare it interoperable before those structures and independent vectors are agreed.

## Test evidence actually produced

- `codec_probe.py` executed in the research sandbox: Python 3.13.5, NumPy 2.3.5, installed lz4 Python and zstd executable. Nine 131,072-sample sources; exact CS12 roundtrip assertions passed; per-source SHA-256 and BFP8 numeric error logged. No per-second codec performance was measured.
- `v1_draft_wire_vectors.py` executed; two tentative frames were generated: 92-byte raw and 89-byte packed with CRC32 fields `68baa8fe` and `07b15062` respectively.
- `validate_bundle.py` generated a separate successful validation log (see file). This validates the sandbox artifacts, **not a wire standard or hardware source**.

## How to reproduce locally

From the extracted `orcstream-agent3/` folder, with Python 3 and NumPy available:

```powershell
# PowerShell or cross-platform shell, run from package root
python -m pip install numpy lz4
python experiments/codec_probe.py
python experiments/v1_draft_wire_vectors.py
python experiments/validate_bundle.py
```

The logs and generated data are saved in `experiments/` beside the programs. The synthetic probe automatically skips zstd if the optional `zstd` command is unavailable. `validate_bundle.py` currently expects zstd rows present in the supplied results and therefore validates the **included** known run; run it on a fresh machine without zstd after retaining included CSV, or install zstd for a full regeneration. It does not require internet or a radio.

## Next authorized engineering task

Review the two competing header designs (v0.1 and Agent 3 v1 discussion draft) and decide whether the 80-byte header overhead is justified; freeze context/clock/subscription TLV syntax and add an independently coded C99 parser on a **new non-protected development branch** only after authorization. Next, verify real signed12 input on the exact Fishball radio and create real multi-vendor IQ fixtures before attempting to market any compression performance.

**No repository changes were made.** This package is intended for owner review and optional future cherry-pick/import; do not merge any unreviewed wire draft into `main`.
