# OrcStream

OrcStream is an experimental, hardware-independent SDR stream protocol and portable C99 library. It transports native complex IQ bytes with explicit sample format, timing validity, sample index, packet sequence and CRC32.

**P1 development branch:** a C99 codec, an allocation-free incremental parser, offline producer/consumer and deterministic tests. All six native formats (CU8, CS8, CS16 LE/BE and CF32 LE/BE) are tested without converting samples. This is **not a stable protocol or production network service**.

Build and test:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Offline IQ demo:

```sh
./build/orcstream_producer input.cs16le out.orcs 3 1024 2400000
./build/orcstream_consumer out.orcs reconstructed.cs16le 3
sha256sum input.cs16le reconstructed.cs16le
```

The protocol core does not depend on SDR hardware, a network, a specific operating system, or dynamic allocation. File I/O in the examples is a separate adapter. Use `docs/P1_VALIDATION_REPORT.md` for test evidence and `docs/P1_REVIEW.md` for limitations. `PROJECT_TRUTH.md` in `main` remains the authoritative release-state source until these changes merge.

OrcSDR and custom Pluto firmware are potential adopters, not build dependencies.
