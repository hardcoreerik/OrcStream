# STATUS — OrcStream

**Observed snapshot: 2026-11-10.** Planning documentation; not a release checklist. [PROJECT_TRUTH.md](PROJECT_TRUTH.md) is the authority on what is implemented. If a tool, build or hardware verification was not run, state that.

## Progress at snapshot

| Area | Evidence label | Details and gaps |
| --- | --- | --- |
| Project identity and research README | MERGED | `main` repository initialized; no production API on main |
| C99 IQ packet codec | EXPERIMENTAL | `chatgpt/protocol-c99-v0.1-20261104` only; six native formats, bounded parser, CRC32, continuity |
| File replay / UDP loopback | EXPERIMENTAL | Synthetic examples/tests on experimental branch; not device traffic |
| ESP32-P4 build and hardware IQ | BLOCKED | No OrcStream-on-P4 integration accepted here |
| Pluto host IIO/USB driver | REPORTED / EXTERNAL | `esp-pluto-sdr` Claude branch; not an OrcStream implementation |
| Pluto + OrcSDR IQ | REPORTED / EXTERNAL | OrcSDR Claude branch; no OrcStream stream verified |
| Native OrcStream output from Pluto | PROPOSED | Custom firmware/USB/Ethernet producer not implemented |
| Lossless IQ compression/packing | PROPOSED | No OrcStream codec or measured ratio |
| Sessions, authorization, fanout | PROPOSED | Not implemented |
| FPGA decimator / FFT / event capture / GPIO | PROPOSED | References exist in Fishball ecosystem; OrcStream job integration not implemented |
| VDL2 / LoRa offload | PROPOSED | No portable job, FPGA block or validated decoder on Pluto |
| Interop adapters (rtl_tcp, VITA, Soapy, SDR++) | PROPOSED | No OrcStream conformance claim |
| Release-ready stable wire | BLOCKED | No v1 compatibility freeze or independent client evidence |

## Active references (not merges)

- OrcStream C99 prototype: https://github.com/hardcoreerik/OrcStream/tree/chatgpt/protocol-c99-v0.1-20261104
- Pluto host driver: https://github.com/hardcoreerik/esp-pluto-sdr/tree/claude/iiod-client-core
- OrcSDR P4 Pluto integration: https://github.com/hardcoreerik/OrcSDR/tree/claude/pluto-p4-core-source
- Original independent driver research: https://github.com/hardcoreerik/esp-pluto-sdr/tree/chatgpt/esp-pluto-sdr-driver

## Next actionable work

**P1: review and harden the portable C99 protocol implementation on a fresh OrcStream development branch**, based on the latest `main`; selectively reconcile the experimental prototype; add reproducible tests and independent receive/send roundtrips. This is the highest-value immediate OrcStream-only engineering task. Do not change firmware or OrcSDR in that phase.

In parallel, a **separate investigation** should map Fishball firmware, IIO/USB gadget, FPGA block diagrams and license/build/recovery process before proposing custom firmware changes.

## Evidence ledger template

For every update record:
- **Date / platform / board and firmware revision / test setup**
- **Repository / branch / commit**
- **Operation and expected result**
- **Measured result, log/artifact and reproduction command**
- **Evidence level:** simulated / host-tested / build-tested / real hardware / independently reproduced
- **Limitations, regressions, blockers and follow-up owner**

Do not treat roadmap entries or downstream anecdotes as completed OrcStream work.
