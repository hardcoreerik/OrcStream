# ARCHITECTURE — OrcStream

Design target; not an implementation claim. Updated **2026-11-10**. [PROJECT_TRUTH.md](PROJECT_TRUTH.md) governs claims.

## What OrcStream is

A hardware-independent, portable SDR **data and control protocol** with a core C99 SDK, platform adapters, optional server/gateway services and clearly defined extension points. It is neither an SDR driver nor a firmware image. Its job is to identify, preserve, transform *with permission*, describe and transfer radio-derived streams to applications.

```text
 SDR sources (RTL / Pluto / other radios / IQ file)
            | generic hardware sample + capabilities API
            v
     Source adapter / optional on-device processing
            | original IQ and separately tagged derived products
            v
     OrcStream packet core + session/subscription contract
            | physical transport adapter
            v
   USB bulk | USB networking | Ethernet | TCP | UDP | file
            v
  PC / Raspberry Pi / ESP32-P4 / gateway / applications
```

An OrcStream *producer* can run on a host or directly on a radio running Linux. The receiver/client may run on another host. A browser needs a bridge; raw UDP is not a browser API.

## Repository boundary and source ownership

| Component | Owns | Must not depend on |
| --- | --- | --- |
| OrcStream | wire, codecs, subscriptions, framing, events, jobs, conformance SDK | OrcSDR, Pluto bitstream, ESP-IDF |
| `esp-pluto-sdr` | portable host driver, IIO/USB/TCP control, IQ/caps/GPIO/trigger transport and scan-format validation | OrcSDR or a mandatory OrcStream stack |
| Optional Pluto custom firmware | Zynq Linux services, FPGA processing, native source-side OrcStream, firmware builds | OrcSDR application-specific assumptions |
| `esp-rtl-sdr` | RTL2832U hardware control/native IQ | OrcStream protocol specifics |
| OrcSDR / other apps | user policy, decoder selection, local DSP and source UI | dictating generic protocol/driver semantics |

Generic processing services (e.g. a decoder implementation adapted from OrcSDR with suitable licensing) may run in Pluto firmware, but are **optional plugins** with their own provenance. Core OrcStream messages must never require them.

## Two Pluto connection paths

**A. Host gateway:** AD9361 -> FPGA/IIO -> USB -> `esp-pluto-sdr` on ESP32-P4/PC/Pi -> generic OrcStream producer -> clients. Valuable for Tab5 UI, GPIO and local forwarding, but may be bandwidth-limited by C6 Wi-Fi.

**B. Native Pluto producer:** AD9361 -> FPGA/DDR -> Zynq Linux OrcStream service -> Ethernet/USB networking/USB bulk -> independent client. ESP32-P4 can control without relaying all IQ. Neither path has been implemented in OrcStream yet.

**Always preserve standard IIO/IIOD** as an independent legacy path. Do not tunnel proprietary packets inside an unmodified libiio USB pipe and claim IIO compatibility. Prototype the native OrcStream producer on Pluto Ethernet/USB IP first; investigate an optional dedicated USB composite bulk interface after verifying gadget descriptors, available endpoints, USB host stack/class support and interference with ordinary IIO.

## Subscription contracts (proposed)

| Mode | Meaning | Original IQ recoverable? |
| --- | --- | --- |
| `RAW_IQ` | Native declared IQ, no implicit format conversion | Yes, subject to disclosed source continuity |
| `PACKED_IQ` | Proven reversible sample packing or codec | Yes, if reconstruction tests pass |
| `CHANNEL_IQ` | Frequency-shifted/filtered/decimated channel | No; derived narrow view |
| `SPECTRUM` | FFT/power bins with window/resolution/calibration | No |
| `EVENTS` | RF activity, measured features, candidate labels/decoded messages | No |
| `TRIGGERED_IQ` | Pre/post-trigger sample window in declared format | Only the captured window |

Audio, telemetry and controls may become separate stream families, never be disguised as raw IQ. A client explicitly opts in to transformations. Bandwidth exhaustion is reported rather than silently switching fidelity.

At 2.4 MS/s single-channel: signed 16-bit IQ requires 9.6 MB/s of payload; packed signed 12-bit IQ could use 7.2 MB/s if the source representation is verified; CU8 would use 4.8 MB/s but may discard precision. These are arithmetic figures, not actual measured network rates. General-purpose lossless compression of noise-heavy IQ may yield little; test before claiming ratios.

## Hardware acceleration control model

Source advertises capabilities from actual firmware/FPGA identity: supported formats, RX channel and shared-LO groups, rate and filter ranges, GPIO pin availability, clock/timebase, supported processing services, concurrency limits, hardware resources, and current availability.

Conceptual lifecycle: `DISCOVER -> REQUEST -> AUTHORIZE -> RESERVE -> CONFIGURE -> START -> OBSERVE -> STOP/RELEASE`. Offload switches use a new stream epoch; a retune/reconfiguration may introduce a gap. Client can choose host-only, source-preferred, or automatic after real cost/throughput measurement. A resource-busy or unsupported response is distinct from successful offload. A loaded FPGA graph can switch features more quickly than a new bitstream; do not promise seamless partial reconfiguration.

FPGA is best for sample-synchronous FIR/decimation, DDC/channelization, FFT/power/thresholds, trigger/clock tagging and selected PHY primitives. Zynq ARM is better for buffer control, network framing, classification rules/models, error correction and decoder framing. Host PC/Pi/P4 may process locally if acceleration is unavailable or not worthwhile.

**VDL2 and LoRa** are candidates, not current features. Begin with channelization and verified signal detection; only then consider named `phy.vdl2` and `phy.lora` optional jobs. AD9361 RX channels share an LO; two RX inputs do not imply independently tuned simultaneous receive bands. Never invent capabilities or resource capacity.

## Timing, provenance and trust

Every IQ frame (and derived event referencing it) needs source/channel, stream ID/epoch, actual rate, center frequency, exact format, sample index/window, declared clock domain and continuity. Producer frame sequence and source sample continuity are separate. A hardware sample counter is not an absolute clock; UTC requires independently validated synchronization and uncertainty metadata.

Classification events must include measured features, algorithm/model version, evidence window, confidence calibration status and allow `unknown`. Decoder packets should preserve checksum/CRC verdict and traceability to the originating IQ window where available. Expose gaps/dropped frames at each stage.

The session control plane authenticates and authorizes control operations separately from IQ CRC. Apply bounded buffers, per-client backpressure, resource quotas, safe GPIO pin masks, fault isolation, USB disconnect recovery and source lifecycle ownership. RX-first: no transmit operations in early delivery milestones.

## Compatibility and evolution

The experimental C99 v0.1 64-byte header supports only IQ/HELLO and is not frozen. Add **versioned** metadata/packet families and capabilities rather than quietly reinterpreting old flags. No direct wire interoperability with rtl_tcp/VITA-49/SpyServer is claimed; support through explicit adapters with reference tests. See [docs/PROTOCOL_EVOLUTION.md](docs/PROTOCOL_EVOLUTION.md).
