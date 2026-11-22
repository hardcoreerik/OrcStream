# OrcStream Competitive Differentiation — evidence before claims

**Research proposal · 2026-11-21.** Positioning is conditional: OrcStream is not currently shown to outperform SDR++, SDRangel, DIFI/VITA packet transport, SoapyRemote, libiio or O-RAN compression. The research question is which *combined* features would cause independent vendors to adopt it voluntarily.

## What competitors already do well

- DIFI/VITA 49 already solve interoperable *digital IF transport/context* in professionally defined domains. Building an unrelated incompatible protocol without credible adapters would increase the burden on users. DIFI latest standards index currently identifies v1.3.0 [D1,D2].
- SDRangel already exposes FLAC/zlib compressed remote IQ and squelch; SDR++ remote features include sample type selection and compression [D3,D4]. “We invented compressed SDR streaming” is false.
- O-RAN BFP/μ-law/block scaling have standardized methods and hardware IP implementations [D5,D6]. Claiming a generic BFP encoder is unique is not defensible.
- ADI libiio and SoapySDR solve device access and streaming for many sources; OrcStream should **adapt**, not demand replacement [D7,D8].
- GNU Radio/ZeroMQ and UHD/RFNoC have established high-performance DSP and FPGA stream architectures [D9,D10]. Do not duplicate their strengths without a measurable gap.

## Real potential differentiators (not yet earned)

| Opportunity | User value (1–5) | Potential (1–5) | Portability (1–5) | Feasibility (1–5) | Complexity (1–5; higher = harder) | Verification required |
|---|---:|---:|---:|---:|---:|---|
| Bit-exact native IQ across tiny and powerful devices | 5 | 4 | 5 | 5 | 3 | same samples/hashes from >=2 vendors and >=3 host classes |
| Explicit fidelity modes A/B/C/D, machine-checkable | 5 | 5 | 5 | 4 | 4 | reference arithmetic + demod/weak-signal evidence, strict fallback |
| Adaptive codec selection with no incompressible-data expansion beyond frame | 5 | 4 | 4 | 4 | 4 | worst-case bounds and same-corpus deadline/wire tests |
| HW-neutral codec portability from C99 to FPGA/GPU | 5 | 5 | 5 | 3 | 5 | independent C and RTL decoded golden vectors, timing closure |
| Sample-accurate origin/epoch and transport gap honesty | 5 | 4 | 5 | 4 | 4 | fault-injected multicast/UDP/network/retune/overrun cases |
| Built-in Soapy/GNU Radio/DIFI bridges | 5 | 3 | 5 | 4 | 4 | open integration demos and standards conformance, no hidden conversion |
| Optional signal-derived products with provenance | 4 | 5 | 3 | 3 | 5 | source sample mappings, filter proof, detector false positives |
| Tiny bounded-memory receiver baseline | 4 | 4 | 5 | 4 | 3 | ESP32-P4 + MCU class measured RAM/CPU/packet limits |
| Multi-channel coherent stream contract | 4 | 4 | 3 | 3 | 5 | hardware-calibrated phase/coherence and timebase uncertainties |
| Open test corpus / interop fixtures / Wireshark dissector | 5 | 4 | 5 | 5 | 3 | third-party independent implementation from specification |

Scores are **Agent 3 prioritization judgments**, not performance test results. Prefer higher real-world value and feasibility with low integration/maintenance burden. No numeric “industry leading” metric follows from this table.

## Candidate claims with required evidence

### Claim A: “Native IQ fidelity is provable, not implied.”

**Precisely scoped future claim:** For `Mode A` with no declared gaps, OrcStream reference decoders reconstruct source-native complex-sample words **bit-for-bit** for each advertised format, from at least two independent sources. **Proof:** original capture hashes, packet golden vectors, independent decoders, property tests, endian/extrema/CF32 bit-pattern cases, clock/gap cases. **Present status:** proposed; unmerged v0.1 already has host tests but no real device integration [D11].

### Claim B: “Compression never silently changes what IQ means.”

**Future claim:** Mode A is exact, B has validated maximum component error, C carries measurement-profile thresholds and D is marked derived. **Proof:** signed sample test vectors, error-bound exhaustive tests, perturbation SNR/weak-signal/phase regression, negotiated consent and rejected unknown modes. **Current:** architecture, not interoperable implementation.

### Claim C: “Adaptive bitstream is more useful than one fixed codec.”

**Narrow future claim:** On a **pre-registered mixed holdout corpus**, an OrcStream selector yields lower *total encoded wire bytes subject to fidelity and p99 latency limits* than each fixed baseline and is never worse than raw packed by more than declared per-frame overhead. **Proof:** same corpus, frozen baselines (raw16, packed12, LZ4, zstd, Rice and SDR++/SDRangel), versioned selectors, CPU/decoder RAM, worst-case expansion. **Counterexample required:** full-uniform 12-bit noise where codec should bypass. **Current:** only preliminary synthetic size experiment—no adaptive selector built.

### Claim D: “Works on MCU, FPGA appliance and server with the same wire.”

**Future claim:** At least Tier 0 MCU-like endpoint, Tier 1 ESP32-P4, Tier 2 Zynq/FPGA and Tier 3 desktop correctly exchange identical Mode A frames within declared constraints. **Proof:** byte-level conformance, no-dynamic-allocation core, endianness/ABI, pinned firmware + FPGA hashes, multiple operating systems. **Current:** unmerged C99 host experiment, no Tier 0/1/2 hardware acceptance.

### Claim E: “Higher useful RF bandwidth over bottleneck links.”

**Legitimate future claim:** On **named measured radio/link/scene** at fixed error limit, OrcStream delivered valid samples/s or derived application utility exceeds raw16 transport, with source and subscriber drop counts equal to zero during the measured interval. **Proof:** USB/Ethernet measured goodput; frozen packet overhead; codec cycle/energy and latency; same source snapshots; exact fidelity or declared metric. **Disallowed:** “Full 61.44 MSPS 12-bit dual RX lossless over USB 2.0” as a universal or presumed capability.

### Claim F: “Reliable coexistence and standards bridges.”

**Future claim:** A Pluto reference appliance offers an optional OrcStream native producer while stock IIOD remains fully usable; DIFI/VITA and Soapy clients see properly mapped streams. **Proof:** stock client regressions, known-good firmware recovery, independent DIFI parser/certification where feasible, clock context mapping, sample hash and gap injection. **Current:** future work, separate from any FPGA offload assertions.

## Adoption strategy

**Licensing:** Independent permissive C99 core and formal public packet registry; every borrowed library/source/HDL and codec patent needs review. Publish SBOM and dependency matrix, not vague MIT claims covering copied firmware derivatives.

**API design:** Single `orcstream_encoder_init() / encode() / decode() / get_caps()`-style stable C ABI after review, caller-owned scratch buffers, optional C++ RAII and Python bindings, portable versioned sample and context objects. Expose errors and losses prominently. No OrcSDR-only types.

**Onboarding:** `make test`/CMake/CTest, 5-minute CPU-only file-to-file quick start, Wireshark dissector, deterministic wire vectors, two small independent receiver examples, and separately installable Soapy and GNU Radio adapters. Publish one interoperability matrix showing actual tested firmware/software versions.

**Governance:** Public RFCs for new codec IDs, reviewable wire and quantization changes, a conservative major-version policy and shared conformance suite. Avoid incompatible private manufacturer forks by reserving ranges and requiring semantic version info.

**Credible language:** “Designed to…”, “in draft…”, “measured on fixture X…”, “no general lossless ratio guarantee.” Avoid “state-of-the-art,” “industry first” and “X times faster” until all experimental and production evidence is in hand.

## Competitive success criteria and stop rules

1. Reject a candidate codec if a simpler baseline wins after *codec parameters + framing + CPU/p99 constraints* on most relevant heldout traces.
2. Reject lossy mode for coherent measurement profiles if unbounded cross-channel phase or signal detection errors remain.
3. Do not build FPGA-specific wire fields or require client vendor hardware; optimized backends may differ but decode semantics cannot.
4. If adaptive selection spends more CPU/energy than network savings on the target link, default to exact packing/identity for that hardware.
5. If external standards already solve a customer's use case, deliver a conformance-tested adapter rather than “replace their stack” advocacy.

## Source links

[D1] https://dificonsortium.org/standards/

[D2] https://vita.com/page-1855471

[D3] https://github.com/f4exb/sdrangel/blob/master/plugins/channelrx/remotetcpsink/readme.md

[D4] https://www.sdrpp.org/manual.pdf ; https://github.com/AlexandreRouma/SDRPlusPlus

[D5] https://www.etsi.org/deliver/etsi_ts/103800_103899/103859/07.00.02_60/ts_103859v070002p.pdf

[D6] https://docs.altera.com/r/docs/683238/25.1.1/o-ran-ip-user-guide/o-ran-ip-functional-description

[D7] https://developer.analog.com/docs/libiio/main/iiod/

[D8] https://github.com/pothosware/SoapyRemote/blob/master/client/Streaming.cpp

[D9] https://wiki.gnuradio.org/index.php/ZMQ_PUB_Sink

[D10] https://uhd.readthedocs.io/en/latest/page_rfnoc_fpga.html

[D11] https://github.com/hardcoreerik/OrcStream/blob/27c134060631a2efb74da04ee760e49957474d96/HANDOFF.md
