# P1 experiment review and protocol contract

**Reviewed:** `main` at `66c3a294d36c2ad1b38243b9146952cabb5aa22f` against `chatgpt/protocol-c99-v0.1-20261104`, 2026-11-10. The branches diverged: experiment ahead 2 / behind 1, with no C99 library on current main. Source/protocol design selected from the experiment, retaining MIT provenance (Oregon Radio Company LLC). This P1 branch remains unmerged and experimental.

## Retained implementation decisions

- Fixed 64-byte `ORCS`/v0.1 header and CRC32 over header bytes 0–59 followed by IQ bytes, CRC stored at offset 60.
- Big-endian integer fields in the header and explicit native payload byte order; six format IDs, 1..6; direct `memmove` of payload bytes, no float conversions.
- Borrowed payload pointers, bounded stream reader with caller-owned storage, no allocator in core, one tracker per stream ID.
- Original HELLO, packet codec, statuses and sample-index contract. No network transport or SDR driver in library.

## Problems found and hardened

- Sequence at `UINT64_MAX` previously wrapped in tracking: reject it on encode/decode. New stream ID is needed before exhaustion. This is stricter validation of experimental v0.1.
- Reader previously reset itself after corrupt header/payload, allowing implicit continuation after a malformed frame. Reader now enters sticky failure, requiring explicit `orcstream_reader_reset` and a trusted frame boundary.
- Ordinary IQ metadata (format, rate, center-frequency value and validity, clock domain when supplied) now must not change without a declared discontinuity. This check is in the tracker, not in the stateless packet decoder.
- Repeated packets after EOS are rejected by the tracker. The stand-alone consumer also requires HELLO, initial index/sequence zero, one stream, strict continuity, and an explicit EOS and complete final frame.
- Added independent Python fixture generator and byte/hash oracle against C producer and C consumer; negative streams cover header corruption, truncation, drop, reorder, duplicate and multiple IDs.

## Important compatibility limitations

The wire header length and field offsets stay at **draft v0.1**. The **C tracker/reader structure layout and error statuses changed**, so do not treat this as an ABI-stable update. Receivers using old permissive tracker behavior may now reject streams. A sender can assert `DISCONTINUITY` to reset its sequence/index/metadata; without epoch numbers or authentication, the receiver cannot prove that this is an authentic, not replayed, source reset. The consumer treats an unexpected gap as failure and does not reconstruct missing samples.

A physical EOF alone cannot prove that a live stream has ended: the offline consumer requires an explicit EOS. The stateless parser permits HELLO and IQ ordering independently; session order is enforced by a higher-level consumer. No multi-stream mux implementation or reorder buffer is supplied; run a tracker per stream, and never silently interleave separate streams into one recording.

An encoder caller must provide a valid payload object that does **not overlap the destination header**. Its output payload pointer may alias the input after decoding; consumers must process it before the next reader feed. Frames up to 65,600 bytes require corresponding caller-owned storage; typical UDP examples should use much smaller frames to avoid IP fragmentation. The maximum includes no transport-specific protection. No control-plane authentication or confidentiality is implemented.

## Future version extension points — proposal only

Do **not** freeze v0.1 as v1.0. A future incompatible draft should use a revised major/minor version and an extensible length-delimited header or validated TLV registry, with explicit unknown-critical-field behavior. Proposed bounded metadata: source/channel identity and stream epoch; acquisition/drop provenance; clock origin and uncertainty; payload family and original sample window; transform/codec ID with decoded format, exactness and maximum output; HELLO capability negotiation. Keep source/transport/provenance/jobs separate. Wire version changes need golden byte vectors and old/new receiver rejection tests. No compression, packed CS12, events, spectrum, jobs, sessions, or hardware adapters were implemented in P1.
