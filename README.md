# OrcStream

**OrcStream** is an emerging, standalone, open-source SDR data and control protocol project. Its goal is to connect radio hardware, SDR applications, browsers, and gateways through a portable, fidelity-aware protocol.

## Project status

**Pre-specification / research.** No protocol version, wire format, performance claim, or compatibility guarantee has been released.

## Design principles

- Preserve the original samples when full-fidelity IQ is requested. No silent quantization, decimation, or drops.
- Separate radio acquisition and hardware drivers from network transports and application dashboards.
- Support explicit subscriptions for full IQ, channel IQ, spectrum, audio, decoder events, telemetry, and controls.
- Make sample timing, epochs, retunes, and known versus unknown gaps observable without inventing RF timestamps.
- Keep control authority and authentication explicit; default to safe local deployment.
- Prefer portable implementations with bounded memory and independently reproducible tests.
- Keep established SDR client compatibility available through adapters, not by corrupting existing protocols.

## Intended first integrations

- [OrcSDR](https://github.com/hardcoreerik/OrcSDR)
- [esp-rtl-sdr](https://github.com/hardcoreerik/esp-rtl-sdr) (independent hardware driver)
- OrcSDR Web and future desktop/Linux gateway clients

Protocol and licensing decisions will be documented before a production API is declared stable.
