# Pluto reference accelerator — OrcStream integration plan

Updated **2026-11-10**. **This is a proposal; not implemented.** For current truth see [../PROJECT_TRUTH.md](../PROJECT_TRUTH.md). The goal is a **universal programmable SDR coprocessor** reference, not an OrcSDR-only firmware.

## Two independent code products plus common protocol

1. **`esp-pluto-sdr`**: standalone, portable host/ESP32-P4 hardware driver. Source selection, USB/IIOD/TCP handling, scan-layout validation, IQ reception, frequency/rate/gain, basic GPIO/trigger control, firmware and processing capability discovery. No direct dependency on OrcSDR or OrcStream in the core driver.
2. **Optional Pluto companion firmware/FPGA** (separate repository and license/baseline decision): Linux service on Zynq ARM, custom FPGA data path/HDL, safe firmware upgrade/recovery, IIO coexistence, optional native OrcStream producer. It may reuse appropriately licensed generic OrcSDR decoders *as optional services*, but must not require OrcSDR.
3. **OrcStream**: independent protocol and SDK defining portable streams, capabilities, control jobs, metadata, transport and fidelity semantics. Its producer adapter may run on Pluto ARM or an external host; no Pluto-specific assumptions in common packet formats.

The first customers are custom Pluto firmware and OrcSDR, but PC, Raspberry Pi, ESP32-P4/Tab5, Waveshare, Linux gateways and other SDRs must all be able to adopt OrcStream without proprietary application coupling.

## Reference hardware and existing investigations

- Zynq-7020 / AD9361 Fishball-derived Pluto clone; specific firmware variant and board revision require recording before changes. Upstream Fishball firmware and FPGA work: https://github.com/matsvandamme/fishball7020-fpga-devkit
- Existing separate host work: https://github.com/hardcoreerik/esp-pluto-sdr/tree/claude/iiod-client-core
- Existing OrcSDR P4 app integration (consumer only): https://github.com/hardcoreerik/OrcSDR/tree/claude/pluto-p4-core-source
- Unmerged earlier esp-pluto-sdr research/driver branches should be reconciled, not blindly discarded. The existing host work reports useful USB rates but still has open transfer lifecycle, hot-unplug, ownership, input-range and audio-capacity concerns, which are **not** evidence of OrcStream stability.
- Check device's shipped operating system, U-Boot, kernel/IIO, USB gadget functions, PL bitstream, version, connector pinout and available DDR against actual hardware. Research does not establish universally identical clone characteristics.

## Keep standard IIO and offer optional OrcStream

**No forced migration.** A non-OrcStream application should retain ordinary libiio/IIOD operations and native IQ. Enhanced firmware advertises a distinct OrcStream service, first over existing network transport (wired Ethernet or verified USB networking), later potentially a dedicated composite USB bulk interface. Do not break firmware update, console/serial or IIO endpoints. Composite enumeration, VID/PID/interface allocation, full-/high-speed host capabilities and Windows/macOS/Linux/ESP-IDF driver support are acceptance concerns, not assumptions.

Two producer variants:
- **Host-mediated:** Pluto USB -> portable `esp-pluto-sdr` -> ESP32-P4 or PC/Pi OrcStream adapter -> clients.
- **Source-native:** FPGA/AD9361 -> Zynq Linux OrcStream producer -> Ethernet or USB -> client; P4 may act only as controller. Prefer this for high-bandwidth when host relaying is wasteful.

Negotiate raw CS16 first (native bit representation and scan endianness verified), not an ad-hoc protocol hidden in IIO payload.

## Capability inventory (proposed)

| Feature | Location | Interface and verification |
| --- | --- | --- |
| Source frequency/rate/gain/native scan format | IIO driver | Actual readback, ranges, rejected controls, precise effective rate |
| FPGA FIR/decimator | FPGA + Linux control | Measured DMA output sample rate and filter alias rejection |
| Dual synchronized RX | AD9361/FPGA | Channel IDs, interleaving and alignment; shared RX LO grouping |
| Packed signed 12-bit IQ | FPGA or Zynq ARM | Bit-exact reconstruction against native signed 12-bit data |
| FFT, power spectrum, detector | FPGA | Bin parameters/calibration, event source sample windows and false-positive testing |
| Triggered pre/post IQ capture | FPGA counters + DDR + Linux | Exact offsets, capacity/overflow, trigger latency, invalid window handling |
| GPIO and sample-synchronous triggers | Zynq ARM + FPGA and optional P4 GPIO | Safe voltage/direction, pin capabilities, polarity, edge/sample counter correlation |
| VDL2 optional processing service | FPGA preprocessing + ARM software decoder | Known reference IQ, valid frames, checksum and missed-frame metrics |
| LoRa optional processing service | FPGA chirp primitives + ARM decoder | SF/BW coverage, channel conditions, payload validation, false positives |
| Health/firmware/bitstream identity | Linux control service | Temperature, clock, DMA overflow, build identity, support matrix |
| Native OrcStream-over-USB | Firmware USB gadget + cross-platform hosts | Independent client plus stock IIO remains functional |

Do **not** assume both independent AD9361 RF channels can tune arbitrary unrelated bands; they share RX LO and have device/bitstream restrictions. A "decoder installed" flag is distinct from "has free resources now." Support explicit unavailable/busy results.

## Data and compression plan

- **Original IQ:** preserve exact native samples and source timing; never call quantized or decimated output raw full-fidelity data.
- **Reversible:** compress native sign-extended 12-bit IQ into 24 bits per complex sample only after measuring exact signed representation and tests across extrema/block boundaries. Ordinary noise may defeat entropy codecs; benchmark CPU/FPGA cost and worst-case expansion before enabling automatic choices.
- **Signal-aware:** DDC/channel IQ, decimation or FPGA FFT/power may lower data needs dramatically **only when requested**. Preserve original sample index ranges and explicit transform definitions.
- **Classified/events:** FPGA measures signal features and triggers; ARM/P4/PC infers likely modulation and decodes. Include evidence windows, decoder version, unknown/confidence, integrity and ability to request IQ when buffered. Event-only transport is not reconstructible as IQ.
- **Dynamic offload:** clients can request capability-aware decode jobs (e.g., `phy.vdl2`, `phy.lora`) through a generic job API. Decision engine compares bandwidth, CPU, latency, resource reservations and failure behavior. Do not put Pluto-specific decoder functions in the generic OrcStream wire core.

At 2.4 MS/s CS16 the minimum payload is 9.6 MB/s; packed signed-12 would be 7.2 MB/s; CU8 4.8 MB/s (lossy from 12-bit). At 960 kS/s **source-side** CS16 IQ is 3.84 MB/s, whereas host-side decimation from 2.88 MS/s still transfers 11.52 MB/s. These are mathematical payloads; do not claim tested USB throughput from them.

## GPIO / physical integration

The Fishball family exposes programmable Zynq GPIO/PL paths. Verify the actual Pluto clone JP headers and FPGA pin mapping before wiring; document voltage domain, boot defaults, reserved pins and whether signal uses PS/Linux or PL sampling. Tab5 and other P4 boards expose different GPIO resources. Host GPIO use requires a per-board adapter; no fixed Tab5 pin assumptions in `esp-pluto-sdr` or OrcStream.

Test common ground, direction collision, edge polarity, pull configuration, reboots and safety. An FPGA input sampled relative to an RF sample counter can be deterministic; a Linux callback or network/USB command cannot be assumed sample-synchronous. For time-of-arrival, a sample counter is not UTC without calibrated clock/reference sync.

## Compatibility, security and licensing

Keep ability to boot known-good original firmware; back up flash/SD partition and original FPGA image. Changes to kernel/libiio/device tree/USB gadget may break other host software. Resource reservation must handle competing IIO and OrcStream consumers and RF-wide filter/LO retunes. Remote FPGA reconfiguration, GPIO outputs and decoder jobs require authorization; RX-only first.

Review Fishball and upstream ADI HDL/Linux/libiio licenses, build/release instructions, artifact provenance and any reused OrcSDR decoder components. OrcStream protocol code can remain independently MIT, but firmware/HDL derivatives may have different obligations. Do not ship unreviewed binaries or unsupported wideband TX.

## Planned sequence

1. **Reconnaissance (no firmware modifications):** capture exact Fishball fork/revision, boot/recovery methods, CPU/DDR and FPGA resource summary, iiod version, USB gadget config, GPIO, available filters, build toolchains and licenses.
2. **Software-only native OrcStream:** cross-compile experimental portable C core for Zynq, capture original CS16 via IIO, send over Ethernet and verified USB IP to independent PC/Pi receiver; hash/sequence proof and IIO coexistence.
3. **ESP32-P4 and PC host adapters:** capability-query and OrcStream control adapter; portable libusb/POSIX where appropriate; native P4 USB host error recovery.
4. **USB bulk interface:** only after host matrix and gadget viability tests; maintain original IIO and USB functions.
5. **FPGA reduction:** filter/decimation, 12-bit packing, output rate readback and spectral fidelity against reference captures.
6. **GPIO/event capture + dual RX:** sample counters, trigger windows, safe GPIO, paired channel metadata.
7. **Processing jobs:** FFT/signal detection, resource-aware offload, then VDL2/LoRa feasibility and known-IQ validation.

## Acceptance must include

Independent non-OrcSDR PC/Pi host demonstrates raw-IQ reception; ESP32-P4 has optional support; original IIO users continue functioning; loss counters and source/sample continuity are truthful; reconfiguration and unplug do not corrupt task/USB lifetimes; firmware can safely recover to known baseline; all feature claims tie to build hashes and named measurements. See [VALIDATION.md](VALIDATION.md).
