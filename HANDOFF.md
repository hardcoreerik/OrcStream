# OrcStream v0.1 experimental SDK — local handoff

**Branch:** `chatgpt/protocol-c99-v0.1-20261104`  
**Repo:** `hardcoreerik/OrcStream`  
**Scope:** independent experimental packet codec and demonstrations, NOT a stable standard, not yet OrcSDR firmware integration.

## Changes

- Network-independent `include/orcstream/orcstream.h` and `src/orcstream.c` C99 static SDK; fixed 64-byte wire header and strict parsing.
- Six explicitly encoded complex IQ formats (CU8, CS8, CS16 LE/BE, CF32 LE/BE).
- Sample sequence, sample index, rational sample rate, optional RF frequency and timestamps, CRC32, reader with caller-provided memory, HELLO, gap tracker.
- Deterministic file capture/replay and local IPv4 UDP example with payload sample verification.
- CMake, CTest, Linux/Mac/Windows CI, protocol draft and MIT license.

## Branch checkout (no merge)

```powershell
git clone https://github.com/hardcoreerik/OrcStream.git
cd OrcStream
git fetch origin chatgpt/protocol-c99-v0.1-20261104
git switch --track origin/chatgpt/protocol-c99-v0.1-20261104
```

If already cloned, run `git fetch origin` then `git switch chatgpt/protocol-c99-v0.1-20261104` when that local branch already exists, or use the `--track` command above.

## How to build and test on Windows (Visual Studio)

PowerShell, from the repo root:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure 2>&1 | Tee-Object -FilePath .\orcstream-test.log
.\build\Release\orcstream_demo.exe write .\synthetic_iq.orcs 48
.\build\Release\orcstream_demo.exe verify .\synthetic_iq.orcs
```

The log `orcstream-test.log` is saved in the directory you ran the commands from (repo root). The `.orcs` file is synthetic, not a real recorded signal. The test automation uses local UDP loopback only.

For single-configuration GCC/Clang/Linux:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure 2>&1 | tee orcstream-test.log
./build/orcstream_demo write synthetic_iq.orcs 48
./build/orcstream_demo verify synthetic_iq.orcs
```

For UDP manual test in two terminals (binary may be under `build/Release` on Windows):

```text
Terminal 1: orcstream_udp_demo receive 7355 48
Terminal 2: orcstream_udp_demo send 127.0.0.1 7355 48
```

Expected: 48 IQ packets and zero missing packet/sample counters on a clean loopback run. Do not treat the synthetic test as a proof of fidelity across real SDR hardware or lossy networks.

## What passed in ChatGPT's sandbox

- GCC 14.2.0 / CMake 3.31.6; Linux Debug and Release build.
- `ctest`: 3/3 passing (core API, UDP loopback, synthetic file replay).
- ASan + UBSan Debug build: 3/3 passing.
- Remote branch pushed and representative Git blob hashes matched local source.

## What has NOT been validated

- Windows/macOS CI result (workflow configured, not previously executed here).
- ESP-IDF or ESP32-P4 compiler and real RAM/CPU results.
- Actual RTL-SDR CU8 sample capture, clock drift, sample timing, device metadata.
- Multiple clients, dynamic subscription, adaptive flow control, reliable network delivery, cryptographic authentication or transport confidentiality.
- VITA-49, Soapy, rtl_tcp or SDR++ interop. No benchmark can show superiority until those are built and compared under identical conditions.

## Next implementation gates

1. Run Windows and ESP32-P4 build, measure bounded memory/CRC throughput before any wire freeze.
2. Introduce authenticated optional control-plane protocol and reliable capability negotiation, separate from IQ data plane.
3. Add stream subscription/session control and multi-subscriber bounded rings, with explicit slow-client policies.
4. Implement **native CU8** OrcSDR producer (using existing hardware driver) and an independent PC receiver; keep driver and protocol in separate repos.
5. Add controlled network impairment benchmark, capture hashes, packet-loss accounting and first-sample latency measurements.
6. Implement VITA-49 receive/emit adapter and standards fixtures, then other client adapters.
7. Only benchmark vs SDR++ after both reference paths and codecs are verified for equal input signal and equivalent fidelity.

Do not merge this PR into main without owner approval. This is an experimental draft and version 0.x may change.
