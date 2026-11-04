# OrcStream

**Built out of spite. Engineered to be undeniable.**

OrcStream is a standalone, open-source SDR **data transport protocol and portable SDK** experiment. The goal is to connect radio hardware, SDR applications, browsers, and gateways without tying IQ transport to any application or device-specific driver.

## Project status: experimental v0.1 C99 core

This branch contains the **first working implementation**, not a production protocol or a frozen standard. No performance superiority, VITA conformance, cloud-scale fanout, lossless compression, or universal hardware support is claimed.

Implemented:

- Fixed 64-byte network-endian wire header, `ORCS` magic, explicit version, sample format, stream ID, per-stream sequence, complex sample index, rational sample rate, optional timestamp/timebase and RF frequency.
- CU8, CS8, CS16 LE/BE, CF32 LE/BE **byte-preserving** IQ packets. No implicit sample conversions or precision loss.
- Bounded 64 KiB maximum payload, CRC32 accidental-corruption detection, strict header/length/flag validation, no allocation in protocol core.
- A caller-owned incremental byte-stream reader, datagram decoder (exactly one frame), HELLO capability payload, and sequence/sample gap tracker.
- Portable deterministic IQ file round-trip and separate loopback UDP transport example; CTest unit, malformed frame and smoke tests.
- Linux/Mac/Windows CMake setup (Windows CI coverage is configured; do not assume it has passed before CI reports).

Not yet implemented: authentication/encryption, robust sessions or discovery, subscription/fanout server, transport congestion/retry logic, device controls, sample/clock synchronization, codec negotiation, compression, VITA-49 and legacy adapters, real SDR hardware integration. UDP demo is intended for loopback only.

[Read the v0.1 wire-format draft](docs/wire-v0.1.md) for the precise offsets, framing, validity rules, performance scope and limitations.

## Build and test

Requirements: CMake 3.16+, C99 compiler, Python 3 (optional for UDP integration smoke test). No third-party library dependency for the core.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Tests execute without SDR hardware. The build generates:

- `liborcstream.a` / `orcstream.lib`: portable static SDK.
- `orcstream_tests`: wire codec, malformed input, fragmentation, formats, sample-gap tracking, pseudorandom decode smoke.
- `orcstream_demo`: write and verify a file of deterministic CU8 IQ packets.
- `orcstream_udp_demo`: send and receive OrcStream frames over loopback UDP, with CRC and sample checks.

### IQ file demo

```sh
./build/orcstream_demo write synthetic_iq.orcs 48
./build/orcstream_demo verify synthetic_iq.orcs
```

On Windows with Visual Studio's multi-config generator use `./build/Release/orcstream_demo.exe` (PowerShell: `.\build\Release\orcstream_demo.exe`). The generated sample file is synthetic; no RF signal is captured.

### UDP loopback (two terminals)

Terminal A:

```sh
./build/orcstream_udp_demo receive 7355 48
```

Terminal B:

```sh
./build/orcstream_udp_demo send 127.0.0.1 7355 48
```

The receiver binds 127.0.0.1 only, never a public interface. Packet order/delivery for general networks is not guaranteed by UDP; this demo verifies **one local run**, not lossless operation on real networks. `tests/udp_loopback.py` automates the same loopback smoke test in CTest.

## SDK usage

Include `orcstream/orcstream.h`; link `OrcStream::orcstream` from the CMake subproject, or link the static archive. Fill an `orcstream_packet`, select format and metadata validity flags, call `orcstream_packet_encode()` into a caller-provided output buffer, and pass the bytes through a transport adapter. At the other end call `orcstream_datagram_decode()` or feed fragments into a caller-owned `orcstream_reader` and process output before the next feed. Encoder/decoder **borrow** IQ payload memory; they never allocate or free application data.

See [the complete file-replay example](examples/iq_file_demo.c), [the UDP adapter](examples/udp_loopback.c), and [the SDK ABI header](include/orcstream/orcstream.h). Explicitly read return codes before relying on a decoded packet.

## Design principles

- Preserve original samples when full-fidelity IQ is requested. No silent quantization, decimation or drops.
- Separate acquisition/driver, packet, transport, remote control and application/dashboard layers.
- Support future subscriptions for full IQ, channel IQ, spectrum, audio, decoder events, telemetry and controls.
- Make sample timing, epochs, retunes, and known versus unknown gaps observable without invented RF timestamps.
- Keep control authority and authentication explicit; default to safe local deployment.
- Support portable bounded-memory implementations with reproducible tests.
- Add established SDR client compatibility via adapters rather than altering those existing standards.

## Intended next integrations

1. `OrcSDR` and `esp-rtl-sdr` at native CU8 with hardware/driver provenance, loss counters, and real SDR benchmarks.
2. TCP transport adapter + session and capability negotiation. A genuine fanout service will follow, including slow-client isolation.
3. PC/Linux gateway and browser bridge. The browser does not natively consume raw UDP.
4. VITA-49/VRT, rtl_tcp, SoapySDR and SDR++ client adapters with interoperable tests.
5. Lossless and optional explicitly lossy codecs only after independent IQ-fidelity and rate/CPU benchmarks.

OrcStream remains independent of OrcSDR, just as independent drivers should be reusable by any application.

## Research and license

Design choices are informed by pinned SDR++ source audit in the private `hardcoreerik/REA-Results` research repository (draft PR #7), not by importing SDR++'s GPLv3 implementation. This implementation is independently authored and licensed under the MIT License; third-party adapters must respect their respective licenses.
