# Validation and acceptance plan — OrcStream

Dated **2026-11-10**. Planned gates, not test results. Keep build logs, capture fixtures and measurement scripts in the corresponding implementing repository/branch; do not insert unrun tests into evidence.

## Contract matrix

| Area | Evidence required | Release blocker |
| --- | --- | --- |
| Native IQ fidelity | Exact byte/sample reconstruction for each declared format and endian; source scan-layout provenance; reference file hashes | Raw IQ silently modified, reordered or undocumented |
| Codec fidelity | Bit-for-bit source reconstruction for lossless packing; quantitative metrics and explicit opt-in for lossy | Lossy stream mislabeled raw/lossless |
| Integrity and parsing | Malformed/fuzzed/fragmented frames, bounded memory, lengths, CRC tests, version reject, no overflow | Unbounded alloc, unsafe parser or accepted invalid frames |
| Continuity | Independent sequence gap, sample gap, source overrun, transport drop and consumer drop accounting | Hidden loss, invented sample indices or clock conversion |
| Portability | C99 host builds and tests (Linux/Windows/macOS where possible), ESP-IDF P4 compile, Raspberry Pi client/producer | Platform assumptions in core or unavailable adapter |
| Transport | TCP/UDP/file and target USB paths under throttle/unplug/replug; end-to-end latency/jitter, backpressure | Deadlock, stale transfer reuse, stream corruption |
| Control security | Authn/authz, replay/failure tests, permission scopes, safe GPIO/firmware operations | Public unauthenticated writes or implicit TX |
| FPGA DSP | Golden IQ impulse/tones/chirp/noise vs reference FIR/FFT, measured PS/PL resource/timing, filter suppression | Wrong sample rate, aliasing hidden, bitstream incompatibility |
| Event classification | Labeled reference IQ, sensitivity/false-positive metrics by SNR, evidence window/model version | Uncalibrated certainty claim, fabricated identity |
| Cross-SDR | At least RTL and Pluto/other source, same SDK consumer, capability fallback tested | Pluto-only wire format |
| IIO compatibility | Stock libiio tune/IQ remains functional with optional enhanced firmware, with rollback | OrcStream-only firmware without documented opt-in |
| Adoption/interop | Third-party host/client, adapters to rtl_tcp/VITA or other protocols with fixtures | Unsupported compatibility claim |

## Standard performance report

For every benchmark report:
- exact repo/branch/commit; firmware image and FPGA build hash; board revision, USB bus speed/topology, host OS, power and clock setup
- RF input or reproducible IQ fixture; channel count, sample format/bit depth, actual sample rate, tune/LO, gain and bandwidth
- input and output payload MB/s, USB/transport throughput (including framing), CPU load per core, FPGA LUT/BRAM/DSP, DMA/PSRAM/DDR usage, power and temperature
- acquisition drops, producer drops, per-subscriber drops, sample/packet gaps, first-sample latency, steady-state latency/jitter
- lossless hash match or measured error/SNR and filter alias rejection; test duration and logs; conditions and uncertainty

Report **source-side IQ** separately from **subscriber-delivered IQ**. A USB receive rate that includes discarded sample blocks is not a successful delivered-rate measurement. Never compare CPU or bandwidth measurements with unequal sample representations as if equivalent.

## Early smoke sequence

1. Reproduce experimental OrcStream C99 host CMake and CTest; record toolchain and outputs.
2. Run byte-exact synthetic file replay/loopback and malformed packet corpus; sanitizer/fuzz and coverage checks.
3. Add independent native CS16 sender/receiver built from the portable SDK, then real source IQ and verified endianness.
4. Test bounded network faults: disconnect, fragmentation, reorder, loss, slow subscriber and restart/epoch consistency.
5. Validate Pluto firmware fallback and IIO coexistence before any special bulk endpoint or FPGA changes.
6. Test safe GPIO and trigger offset capture against a known signal; validate dual-channel alignment only where hardware supports it.
7. Compare host and device acceleration under *equivalent quality* and realistic P4 Wi-Fi, Ethernet and USB paths.
8. Run sustained testing with saved logs and exact files. Retain failure results; never delete evidence merely because later changes improve performance.

## Definition of complete

A feature is complete only when the code, intended-platform build, functional tests, negative/error tests, documentation and evidence all agree. FPGA/firmware features require recoverability and hardware proof. Cross-platform claims require at least one verified implementation per claimed platform.


## P1 host-test evidence (2026-11-10, unmerged)

See [P1 validation](P1_VALIDATION_REPORT.md) for a reproducible offline C99 producer/consumer, independent Python IQ fixtures, malformed-frame tests, sanitizer/fuzz runs, CPU-time microbenchmarks and memory bounds. The branch supplies `tests/run-p1.sh` that writes `orcstream-p1-test.log` into the current command directory. GitHub Actions subsequently passed all 7 Ubuntu/Windows/macOS Debug/Release and sanitizer jobs ([run 42945149685](https://github.com/hardcoreerik/OrcStream/actions/runs/42945149685)); no real SDR or embedded hardware gate was met.
