# IQ Compression Competitive Landscape — OrcStream Agent 3

**Research snapshot:** 2026-11-21 · **Status:** RESEARCH, not a shipped implementation · **Confidence tags:** `V` source-verified, `M` mathematically derived, `H` hypothesis requiring testing, `U` unknown/not independently evaluated.

## Executive findings

1. **Transport is not compression.** VITA 49/VRT, DIFI, eCPRI, CHDR/RFNoC, IIOD/libiio, SoapyRemote and GNU Radio ZMQ establish different transport/control/metadata boundaries. None should be characterized as an intrinsically superior IQ *codec* merely because its packets carry IQ. The relevant mapping and adapter work differs materially by standard [S1–S11].
2. **O-RAN fronthaul is the strongest standards-based starting point for small deterministic lossy integer IQ encoders.** ETSI TS 103 859 Annex A describes BFP, block scaling and μ-law on *frequency-domain 12-subcarrier resource blocks*. Its block-size, metadata, precision and EVM behavior do **not** automatically generalize to time-domain wideband SDR streams [S4, S5].
3. **Competitors already compress IQ.** SDRangel Remote TCP Sink advertises FLAC and zlib compression and configurable squelch; SDR++ remote streaming offers sample-type selection and compression. OrcStream cannot market the generic presence of compressed IQ as novel [S12, S13].
4. **DIFI has advanced to v1.3.0** according to the consortium's standards page. Avoid treating older v1.1/v1.2.x documents as the latest DIFI standard [S2].
5. The defensible opening is a **portable capability/fidelity/continuity contract** with reversible packing, appropriate adaptive coding, deterministic constrained decoders and an independent conformance suite—not an unsupported claim of universally better compression.

## Comparison matrix

Legend: `N/D` = not directly a compression codec, therefore a compression ratio or codec latency is not defined. `?` = insufficient primary-source evidence in this pass. **No numerical vendor performance comparisons were measured.** License statements are project-level pointers, not third-party license clearance.

| System | Samples / compression | Fidelity | Wire / timing / sync | Channels / compute / memory / acceleration | Reusable implementation, licensing, patents | OrcStream response |
|---|---|---|---|---|---|---|
| **VITA 49.0/49.2 VRT** | IF data plus context and optional command packet families; **N/D** | Defined by selected payload format, not by VRT | Packet data/context, stream ID and timestamp semantics; timing is a primary design purpose | Multiple streams; overhead depends on packetization; no codec compute inherent | VITA standards; spec rights/redistribution conditions require review [S1] | Versioned adapter with lossless mapping of representable fields; never claim binary compatibility |
| **DIFI 1.3.0** | Constrained interoperable digital-IF packet/profile of VITA family; **N/D** | Depends on represented samples | IP data/context; explicit synchronization and flow-control evolution | Multiple IF streams; CPU primarily packet handling | DIFI consortium normative standard and conformance code; independently check spec/license [S2,S3] | DIFI 1.3 adapter and context mapping, no OrcStream codec in a DIFI field by default |
| **O-RAN WG4 fronthaul** | Fixed IQ, BFP, block scaling, μ-law, modulation-oriented compression | Lossy modes; fixed-point may be exact only given native representation | Ethernet/eCPRI-oriented radio-resource section metadata; PRB granularity | Multiple eAxC/carrier constructs; deterministic integer/FPGA processing; memory usually one block plus framing | ETSI PAS, O-RAN design and Altera FPGA IP; patents/implementation licensing must be checked [S4–S6] | Study portable *algorithm variants*; don't copy O-RAN PRB frame syntax as universal IQ |
| **eCPRI** | Packetized fronthaul services; **N/D** | Application-defined | eCPRI message and radio-transport framing | Aggregated radio streams; compression occurs above/in radio application layer | eCPRI specification with usage conditions [S7] | Recognize as carriage, not codec; bridge requires defined application mapping |
| **IEEE 1914.3 RoE** | Radio-over-Ethernet encapsulation; **N/D** | Application-defined | Ethernet encapsulation/mapping | Multi-radio aggregation, no codec compute inherent | IEEE standard publication; license review [S8] | Relevant as ecosystem transport, not a compression baseline |
| **ADI libiio / IIOD** | Driver native scan bytes; no generic built-in IQ codec established here | Native sample fidelity when configured correctly | Local/network/USB/serial backends; metadata in IIO context/control | Device/channel scan masks and buffers; hardware DMA; no IQ-codec latency inherent | `analogdevicesinc/libiio` permissive-source components and API; verify exact file license/version [S9] | Parallel native OrcStream service or host gateway; retain ordinary IIO interoperability |
| **SoapySDR / SoapyRemote** | Formats negotiated through stream APIs; remote stream transport, not a verified universal compression scheme | Depends on formats/conversion | Remote stream plus device-control calls; UDP/TCP options | Many SDR families; buffering/window/MTU; data-format costs vary | `pothosware/SoapySDR`, `SoapyRemote` source; inspect current exact licenses before adapting [S10] | Native Soapy adapter that requests explicit formats and preserves flags/timing |
| **GNU Radio / ZMQ** | Byte/item stream or messages; **N/D** absent inserted codec blocks | Depends on chosen blocks | ZMQ PUSH/PULL/PUB/SUB, tags may require deliberate mapping | Scales to host DSP; copies/backpressure and HWM matter | GNU Radio open source; transport integration code reviewed separately [S11] | Source/sink blocks and tag-preserving metadata bridge |
| **rtl_tcp** | Typically unsigned 8-bit interleaved IQ; minimal remote control; no standardized compression | Native CU8 if radio delivers CU8 | TCP byte stream, control command channel; limited timing semantics | Single radio style; tiny sender; CPU/network overhead low; no sync guarantee | `osmocom/rtl-sdr` open source under project terms [S14] | Easy CU8 gateway; avoid inventing native sample timestamps |
| **SDR++ Server** | User-selectable sample type and compression; algorithm/version behavior not audited here | Depends on sample type / codec settings | Proprietary-to-SDR++ remote protocol; session controls | Host/server implementation; memory and latency must be benchmarked | `AlexandreRouma/SDRPlusPlus`; dependency on zstd confirmed in build; wire exactness requires source audit [S12] | Fair competitor, identical input/fidelity comparison with a pinned build |
| **SDRangel Remote TCP** | Advertises 8/16-bit choices, FLAC/zlib IQ compression and optional squelch | FLAC/zlib reversible *for represented samples*, squelch/filter/conversion may not preserve source | RTL0 compatibility and extensions; TCP/WSS | Host CPU compression; latency and memory need measurement | `f4exb/sdrangel` plugin open source, integration/license review [S13] | Compare actual encoded IQ, not just a UI bitrate graph; note false equivalence of squelched audio/IQ |
| **Ettus UHD / RFNoC CHDR** | Transport internal to programmable FPGA/host graph; **N/D** | Dependent on block outputs | CHDR stream, time/control packet families | Multi-stream HW pipelines, FPGA payload and scheduling support | UHD and RFNoC spec source; FPGA design/license scope varies [S15] | Borrow stream discipline and resource reservation ideas, not proprietary on-wire assumptions |
| **FLAC/WavPack/other audio codecs** | Integer sequence predictors/entropy; IQ mapping must be chosen | Lossless for coded integer sequences if exact and tested | Codec frames in transport envelope | Host CPU, variable history and buffers; not designed for arbitrary RF semantics | Check codec-specific licenses and framing before reuse [S13] | Baseline candidate on integer IQ; compare separate I/Q channels vs interleaved |
| **LZ4 / zstd / DEFLATE** | Generic byte-oriented lossless codecs | Bit-exact if decoded correctly | Embedded within explicit OrcStream blocks | Fast software pathways; bounded decoder limits must be specified; some FPGA cost high | `lz4/lz4`, `facebook/zstd`, zlib; licensing in respective repositories [S16] | Optional Tier 2+ profiles only when profitable and decoder supported |
| **Learned / proprietary SDR codecs** | Learned transform/predictor or vendor-private implementation | Cannot assume bit-exact or bounded loss | Unknown until disclosed | Training, weights, floating-point and compute costs vary | Proprietary comparisons require reproducible binaries/data/rights; patent check mandatory | Research only; do not assert superiority or make it mandatory |

**Compression-ratio field:** Every `N/D` is intentionally left nonnumeric. For the others, ratios depend on bit depth, RF scene, gain, ADC noise, exact sample formatting and frame size. Cross-vendor numerical ratios are **unknown** until an equivalent-input benchmark; the only unconditional arithmetic ratio here is sign-extended CS12 samples placed into CS16 containers: 32/24 = 1.333x relative to padded storage, a **25% byte reduction**, not entropy compression. This depends on verifying *all* valid samples fit signed 12-bit values.

## Cross-cutting engineering distinctions

### Sample representation and provenance

- A decoder cannot infer whether CS16 samples are full 16-bit, sign-extended 12-bit, 14-bit, scaled, saturated or dithered from the container width alone. Encoding needs explicit `valid_bits`, signedness, endian, channel order, I/Q order, scale convention and source-adapter proof.
- `CF32` cannot generally be recoded as fixed point losslessly. IEEE-754 bit patterns, NaNs and signed zero are relevant to bit-exact representation contracts, even where they are not meaningful radio amplitudes.
- Cross-channel compression can improve performance only if those channels are correlated and mapping is invertible. Coherent RX demands exact sample alignment and phase preservation, not merely identical sample rates.

### Packetization/latency/memory

- Block codecs introduce at least a block-formation delay of `(B-1)/fs` for a streaming encoder collecting `B` samples, before compute/network latency. At 2.4 MSPS, 256 samples take about 0.107 ms; at 61.44 MSPS, approximately 4.17 µs. These are arithmetic lower bounds, not full system latency.
- Longer contexts raise compression potential on correlated data at the cost of buffer size, reset behavior, latency and unpredictable error propagation. Independent blocks with explicit reset provide a realistic fallback for Tier 0/Tier 1 hardware.
- UDP framing must respect path MTU, with encrypted/tunneled overhead also accounted for; TCP introduces stream framing, backpressure and head-of-line blocking. Unreliable arrival and source acquisition overruns are **different** gap causes.

### Interoperability decision tree

1. **DIFI/VRT consumers** → explicit adapter with context, time, sample type and gap mapping; if compressed OrcStream payload has no equivalent in a particular receiver profile, *decode first*.
2. **SoapySDR/libiio consumers** → hardware API adapter; no assumption of on-wire equivalence.
3. **GNU Radio** → native source/sink blocks or file/replay adapter; sample tags must roundtrip under explicit rules.
4. **rtl_tcp/SDR++/SDRangel** → transport bridges with documented sample-format conversion and limitations; no silent channelized/quantized fallback.
5. **DIFI/O-RAN native IP compression** → never claim direct binary interoperability without a conformance-tested, standards-conformant mapping and rights review.

## Sourced observations versus unverified claims

- **Verified**: O-RAN Annex A BFP shared exponent per 12-subcarrier PRB [S4]; SDRangel Remote TCP Sink FLAC/zlib [S13]; DIFI standards index currently lists v1.3.0 [S2]; libiio/IIOD has USB/network/serial paths [S9]; RFNoC CHDR transports streams [S15].
- **Not verified**: sustained throughput, CPU %, relative EVM, actual codec configurations in SDR++ builds, SoapyRemote compression support, vendor-private codec performance, exact memory/FPGA utilization of unrelated projects, or patent clearance.
- **Licensing risk**: open-source availability ≠ permission to embed third-party code into a permissively licensed SDK without conditions. Protocol *ideas* and independently written implementations are safer than copied source, but still require applicable patent/trademark analysis.

## Sources (primary and directly verifiable)

[S1] VITA VITA-49 FAQ: https://vita.com/page-1855471

[S2] DIFI official standards index (v1.3.0 listed): https://dificonsortium.org/standards/

[S3] DIFI Consortium tutorial: https://github.com/DIFI-Consortium/DIFI-Certification/blob/main/DIFI_101_Tutorial.md

[S4] ETSI TS 103 859 V7.0.2 Annex A, O-RAN WG4 Fronthaul CUS PAS (2022), pp. 243–248: https://www.etsi.org/deliver/etsi_ts/103800_103899/103859/07.00.02_60/ts_103859v070002p.pdf

[S5] MathWorks O-RAN block compression function: https://www.mathworks.com/help/5g/ref/nroranblockcompress.html

[S6] Altera/Intel FPGA O-RAN IP details: https://docs.altera.com/r/docs/683238/25.1.1/o-ran-ip-user-guide/o-ran-ip-functional-description

[S7] eCPRI official specification list: https://www.cpri.info/spec.html

[S8] IEEE 1914.3-2023 standards entry: https://standards.ieee.org/ieee/1914.3/10629/

[S9] Analog Devices IIOD documentation: https://developer.analog.com/docs/libiio/main/iiod/

[S10] SoapyRemote transport implementation: https://github.com/pothosware/SoapyRemote/blob/master/client/Streaming.cpp

[S11] GNU Radio ZMQ PUB Sink: https://wiki.gnuradio.org/index.php/ZMQ_PUB_Sink

[S12] SDR++ user guide: https://www.sdrpp.org/manual.pdf ; source tree: https://github.com/AlexandreRouma/SDRPlusPlus

[S13] SDRangel Remote TCP Sink documented features: https://github.com/f4exb/sdrangel/blob/master/plugins/channelrx/remotetcpsink/readme.md

[S14] rtl_tcp source: https://github.com/osmocom/rtl-sdr/blob/master/src/rtl_tcp.c

[S15] UHD RFNoC CHDR packet format: https://uhd.readthedocs.io/en/latest/page_rfnoc_fpga.html

[S16] LZ4: https://github.com/lz4/lz4 ; zstd: https://github.com/facebook/zstd ; zlib: https://zlib.net/

## Next evidence gaps

1. Pin actual commits for SDR++ remote codec and SoapyRemote source, compile and inspect wire payloads rather than relying on GUI manual terminology.
2. Acquire representative simultaneous multi-radio real IQ captures with provenance, including known AD9361 native signed-word behavior and buffering.
3. Measure O-RAN-aligned BFP against time-domain BFP with a comparable error budget, including weak-signal detection and coherent dual RX—not just 16→9 nominal bits.
4. Consult counsel/standards implementer for the actual patent/IPR/licensing constraints before publishing a “standards compatible” formal claim.

## Academic and research implementation comparison (additional prioritized findings)

| Research | Method / fidelity | Applicability and evidence boundary | Implementation/IP concern |
|---|---|---|---|
| **Context-Aware Lossless and Lossy Compression of Radio Frequency Signals** (2023) | Fully Adaptive Prediction Error Coder (FAPEC) combined with adaptive LPC preprocessing and optional signal-aware lossy quantization; explicitly treats interleaved signed16 SDR I/Q | Directly relevant to time-domain generic RF. Predictor order, chunk length and correlation help on some sources; *cannot transfer paper's ratios to our captures*. Study detector false negatives when noise is treated as discardable | FAPEC algorithm/software rights and reproduction license must be checked; paper's documented design supports independent experiment [S17] |
| **Compression of Channelized Wideband Spectrum Samples** (Adaline Lee, Virginia Tech, 2026) | Channelization, bounding-box detection and mixture-distribution quantizer; lossy | Promising source-side channelized RF storage; channelization and selection are **not** full-band bit-exact IQ. The thesis abstract reports improved MSE vs an adaptive vector quantizer under its tested conditions, not a universally superior codec | Thesis text/associated code availability and right to implement unresolved [S18] |
| **Deep-Learned Compression for Radio-Frequency Signal Classification** (Rodriguez et al., 2024) | HQARF learned vector-quantization, task/ModRec-aware evaluation on six modulation classes | Important demonstration that a learned codec can preserve **classifier utility** while phase/spectrogram reconstruction degrades. Not evidence of Mode A/B or wideband source-faithful IQ [S19] | Model training data, weights, runtime and actual redistribution rights need audit |
| **Neural quantizer for fronthaul compression** (Hisano & Yagi, 2025) | FPGA-realized learned nonlinear quantizer; lossy | Demonstrates hardware feasibility of a neural-derived quantizer, and reports performance comparable to an optimized nonlinear baseline for its test context. Does **not** establish value on noisy wideband SDR [S20] | FPGA resources, quantizer weights, patent/reuse review necessary |
| **Delta-Rice detector-data compressor** (Mathews et al., ORNL, 2024) | Lossless decorrelation + Rice entropy coding; FPGA/GPU variants considered | Not SDR-specific; strong comparative technique and engineering reference for noisy digitized analog signals. Any published speed/ratio on neutron-detector data is **not transferable** to receiver IQ without same-data tests [S21] | Review plugin/source licenses and fixed-point implementation rights |
| **O-RAN BFP9 uplink field experiment** (Ahmed thesis, 2025) | BFP9, runtime bitwidth adaptation, O-RAN uplink | Reports negligible performance impact for robust MCS and more significant retransmission/BLER penalties at high MCS on its setup; supports *application-aware* constraints rather than blanket low-EVM claims [S22] | Research method may inspire test design; no binary/RTL license assumed |

[S17] https://pmc.ncbi.nlm.nih.gov/articles/PMC10098950/

[S18] https://vtechworks.lib.vt.edu/items/475fdff7-c942-4a84-ae3d-c6205a510c60

[S19] https://arxiv.org/abs/2403.03150

[S20] https://doi.org/10.23919/comex.2025XBL0016

[S21] https://www.ornl.gov/publication/delta-rice-hdf5-compression-plugin-optimized-digitized-detector-data

[S22] https://www.politesi.polimi.it/handle/10589/243969
