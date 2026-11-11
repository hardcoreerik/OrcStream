/*
 * OrcStream experimental wire protocol / portable C99 SDK
 * SPDX-License-Identifier: MIT
 * WARNING: Version 0.1 is an experimental draft, not a stable wire format.
 */
#ifndef ORCSTREAM_ORCSTREAM_H
#define ORCSTREAM_ORCSTREAM_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define ORCSTREAM_VERSION_MAJOR 0u
#define ORCSTREAM_VERSION_MINOR 1u
#define ORCSTREAM_HEADER_SIZE 64u
#define ORCSTREAM_HELLO_SIZE 16u
#define ORCSTREAM_MAX_PAYLOAD 65536u
#define ORCSTREAM_MAX_FRAME (ORCSTREAM_HEADER_SIZE + ORCSTREAM_MAX_PAYLOAD)
typedef enum orcstream_status {
    ORCSTREAM_OK = 0, ORCSTREAM_NEED_MORE = 1,
    ORCSTREAM_BAD_ARGUMENT = -1, ORCSTREAM_NO_SPACE = -2,
    ORCSTREAM_BAD_MAGIC = -3, ORCSTREAM_BAD_VERSION = -4,
    ORCSTREAM_BAD_HEADER = -5, ORCSTREAM_BAD_TYPE = -6,
    ORCSTREAM_BAD_FORMAT = -7, ORCSTREAM_BAD_FLAGS = -8,
    ORCSTREAM_BAD_LENGTH = -9, ORCSTREAM_BAD_CHECKSUM = -10,
    ORCSTREAM_BAD_CONTINUITY = -11, ORCSTREAM_BAD_METADATA = -12,
    ORCSTREAM_STREAM_ENDED = -13
} orcstream_status;
typedef enum orcstream_packet_type {
    ORCSTREAM_PACKET_HELLO = 1, ORCSTREAM_PACKET_IQ = 2
} orcstream_packet_type;
typedef enum orcstream_sample_format {
    ORCSTREAM_FORMAT_NONE = 0, ORCSTREAM_FORMAT_CU8 = 1,
    ORCSTREAM_FORMAT_CS8 = 2, ORCSTREAM_FORMAT_CS16_LE = 3,
    ORCSTREAM_FORMAT_CS16_BE = 4, ORCSTREAM_FORMAT_CF32_LE = 5,
    ORCSTREAM_FORMAT_CF32_BE = 6
} orcstream_sample_format;
#define ORCSTREAM_FLAG_TIMESTAMP_VALID 0x0001u
#define ORCSTREAM_FLAG_DISCONTINUITY 0x0002u
#define ORCSTREAM_FLAG_END_OF_STREAM 0x0004u
#define ORCSTREAM_FLAG_FREQUENCY_VALID 0x0008u
#define ORCSTREAM_FLAG_CLOCK_MONOTONIC 0x0010u
#define ORCSTREAM_FLAG_CLOCK_UTC 0x0020u
#define ORCSTREAM_FLAG_CLOCK_DEVICE 0x0040u
#define ORCSTREAM_FLAG_CLOCK_MASK 0x0070u
#define ORCSTREAM_FLAG_KNOWN_MASK 0x007fu
#define ORCSTREAM_CAP_IQ_RX 0x00000001u
typedef struct orcstream_packet {
    uint8_t type, format;
    uint16_t flags;
    uint32_t stream_id;
    uint64_t sequence, sample_index, timestamp_ns, center_frequency_hz;
    uint32_t sample_rate_num, sample_rate_den, payload_len;
    const uint8_t *payload;
} orcstream_packet;
typedef struct orcstream_hello {
    uint32_t capabilities, format_mask, max_payload, max_streams;
} orcstream_hello;
/* The encoder is allocation-free; payload must not overlap destination's
 * header, and caller must supply enough valid writable/output bytes. */
orcstream_status orcstream_packet_encode(uint8_t *dst, size_t capacity,
                                          const orcstream_packet *packet, size_t *written);
orcstream_status orcstream_packet_decode(const uint8_t *src, size_t length,
                                          orcstream_packet *out, size_t *consumed);
orcstream_status orcstream_datagram_decode(const uint8_t *src, size_t length,
                                            orcstream_packet *out);
orcstream_status orcstream_hello_encode(uint8_t dst[ORCSTREAM_HELLO_SIZE],
                                         const orcstream_hello *hello);
orcstream_status orcstream_hello_decode(const uint8_t *src, size_t length,
                                         orcstream_hello *out);
size_t orcstream_complex_sample_bytes(uint8_t format);
const char *orcstream_status_string(orcstream_status status);
typedef struct orcstream_reader {
    uint8_t *storage;
    size_t capacity, used, target;
    uint8_t failed; /* sticky on parse/integrity error; reset explicitly */
} orcstream_reader;
/* A malformed frame leaves the reader failed until orcstream_reader_reset().
 * This prevents implicit resynchronization into arbitrary IQ payload bytes.
 * On success, packet->payload aliases the caller-owned reader buffer and is
 * valid only until the next feed or reset. The caller must check reader.used
 * on physical end-of-input to reject incomplete trailing frames. */
orcstream_status orcstream_reader_init(orcstream_reader *reader,
                                        uint8_t *storage, size_t capacity);
void orcstream_reader_reset(orcstream_reader *reader);
orcstream_status orcstream_reader_feed(orcstream_reader *reader,
                                        const uint8_t *input, size_t length,
                                        size_t *consumed, orcstream_packet *out);
typedef enum orcstream_continuity {
    ORCSTREAM_CONTINUITY_FIRST = 0, ORCSTREAM_CONTINUITY_OK = 1,
    ORCSTREAM_CONTINUITY_GAP = 2,
    ORCSTREAM_CONTINUITY_LATE_OR_DUPLICATE = 3,
    ORCSTREAM_CONTINUITY_DECLARED_DISCONTINUITY = 4
} orcstream_continuity;
typedef struct orcstream_tracker {
    uint8_t initialized, ended, format, frequency_valid, clock_domain;
    uint32_t stream_id, sample_rate_num, sample_rate_den;
    uint64_t center_frequency_hz;
    uint64_t next_sequence, next_sample_index;
    uint64_t missing_packets, missing_samples;
    uint64_t late_or_duplicate, declared_discontinuities;
} orcstream_tracker;
/* Track ONE IQ stream ID. Callers maintain independent tracker per stream.
 * Missing packets/samples are saturated counters, not retransmission requests.
 * A declared discontinuity is an explicit unverified producer assertion.
 * A new stream ID is required after EOS or before sequence exhaustion. */
void orcstream_tracker_reset(orcstream_tracker *tracker);
orcstream_status orcstream_tracker_observe(orcstream_tracker *tracker,
                                            const orcstream_packet *packet,
                                            orcstream_continuity *result);
#ifdef __cplusplus
}
#endif
#endif
