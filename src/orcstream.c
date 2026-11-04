/* SPDX-License-Identifier: MIT */
#include "orcstream/orcstream.h"

#include <limits.h>
#include <string.h>

static void put16(uint8_t *dst, uint16_t val) {
    dst[0] = (uint8_t)(val >> 8);
    dst[1] = (uint8_t)val;
}
static void put32(uint8_t *dst, uint32_t val) {
    dst[0] = (uint8_t)(val >> 24);
    dst[1] = (uint8_t)(val >> 16);
    dst[2] = (uint8_t)(val >> 8);
    dst[3] = (uint8_t)val;
}
static void put64(uint8_t *dst, uint64_t val) {
    put32(dst, (uint32_t)(val >> 32));
    put32(dst + 4, (uint32_t)val);
}
static uint16_t get16(const uint8_t *src) {
    return (uint16_t)(((uint16_t)src[0] << 8) | (uint16_t)src[1]);
}
static uint32_t get32(const uint8_t *src) {
    return ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
           ((uint32_t)src[2] << 8) | (uint32_t)src[3];
}
static uint64_t get64(const uint8_t *src) {
    return ((uint64_t)get32(src) << 32) | (uint64_t)get32(src + 4);
}
/* IEEE CRC-32 of bytes 0..59 of the header followed by payload.
 * The CRC field at offsets 60..63 is omitted, not zeroed into the stream. */
static uint32_t crc_step(uint32_t crc, const uint8_t *p, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i) {
        unsigned j;
        crc ^= (uint32_t)p[i];
        for (j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
    }
    return crc;
}
static uint32_t frame_crc(const uint8_t *header, const uint8_t *payload, size_t n) {
    uint32_t crc = crc_step(0xffffffffu, header, 60);
    if (n) { crc = crc_step(crc, payload, n); }
    return ~crc;
}
size_t orcstream_complex_sample_bytes(uint8_t format) {
    switch (format) {
        case ORCSTREAM_FORMAT_CU8:
        case ORCSTREAM_FORMAT_CS8: return 2;
        case ORCSTREAM_FORMAT_CS16_LE:
        case ORCSTREAM_FORMAT_CS16_BE: return 4;
        case ORCSTREAM_FORMAT_CF32_LE:
        case ORCSTREAM_FORMAT_CF32_BE: return 8;
        default: return 0;
    }
}
static orcstream_status validate(const orcstream_packet *p) {
    size_t bytes;
    uint16_t domain;
    if (!p) { return ORCSTREAM_BAD_ARGUMENT; }
    if ((p->flags & (uint16_t)~ORCSTREAM_FLAG_KNOWN_MASK) != 0) {
        return ORCSTREAM_BAD_FLAGS;
    }
    domain = p->flags & ORCSTREAM_FLAG_CLOCK_MASK;
    if (p->flags & ORCSTREAM_FLAG_TIMESTAMP_VALID) {
        if (domain != ORCSTREAM_FLAG_CLOCK_MONOTONIC &&
            domain != ORCSTREAM_FLAG_CLOCK_UTC &&
            domain != ORCSTREAM_FLAG_CLOCK_DEVICE) { return ORCSTREAM_BAD_FLAGS; }
    }
    else if (domain || p->timestamp_ns != 0) { return ORCSTREAM_BAD_FLAGS; }
    if (!(p->flags & ORCSTREAM_FLAG_FREQUENCY_VALID) && p->center_frequency_hz) {
        return ORCSTREAM_BAD_FLAGS;
    }
    if (p->payload_len > ORCSTREAM_MAX_PAYLOAD) { return ORCSTREAM_BAD_LENGTH; }
    if (p->payload_len && !p->payload) { return ORCSTREAM_BAD_ARGUMENT; }
    switch (p->type) {
        case ORCSTREAM_PACKET_HELLO:
            if (p->format != ORCSTREAM_FORMAT_NONE) { return ORCSTREAM_BAD_FORMAT; }
            if (p->flags || p->stream_id || p->sequence || p->sample_index ||
                p->timestamp_ns || p->center_frequency_hz ||
                p->sample_rate_num || p->sample_rate_den) { return ORCSTREAM_BAD_HEADER; }
            if (p->payload_len != ORCSTREAM_HELLO_SIZE) { return ORCSTREAM_BAD_LENGTH; }
            return ORCSTREAM_OK;
        case ORCSTREAM_PACKET_IQ:
            bytes = orcstream_complex_sample_bytes(p->format);
            if (!bytes) { return ORCSTREAM_BAD_FORMAT; }
            if (!p->stream_id || !p->sample_rate_num || !p->sample_rate_den) {
                return ORCSTREAM_BAD_HEADER;
            }
            if (p->payload_len % bytes != 0) { return ORCSTREAM_BAD_LENGTH; }
            if (!(p->flags & ORCSTREAM_FLAG_END_OF_STREAM) && !p->payload_len) {
                return ORCSTREAM_BAD_LENGTH;
            }
            /* Cannot represent a sample index wrapping uint64_t. */
            if ((uint64_t)(p->payload_len / bytes) > UINT64_MAX - p->sample_index) {
                return ORCSTREAM_BAD_HEADER;
            }
            return ORCSTREAM_OK;
        default: return ORCSTREAM_BAD_TYPE;
    }
}

orcstream_status orcstream_packet_encode(uint8_t *dst, size_t capacity,
                                          const orcstream_packet *p, size_t *written) {
    orcstream_status rc;
    size_t full_size;
    if (!dst || !p || !written) { return ORCSTREAM_BAD_ARGUMENT; }
    *written = 0;
    rc = validate(p);
    if (rc != ORCSTREAM_OK) { return rc; }
    if (p->type == ORCSTREAM_PACKET_HELLO) {
        orcstream_hello hello;
        if (orcstream_hello_decode(p->payload, p->payload_len, &hello) != ORCSTREAM_OK) {
            return ORCSTREAM_BAD_HEADER;
        }
    }
    full_size = ORCSTREAM_HEADER_SIZE + (size_t)p->payload_len;
    if (capacity < full_size) { return ORCSTREAM_NO_SPACE; }
    dst[0] = 'O'; dst[1] = 'R'; dst[2] = 'C'; dst[3] = 'S';
    dst[4] = ORCSTREAM_VERSION_MAJOR;
    dst[5] = ORCSTREAM_VERSION_MINOR;
    put16(dst + 6, ORCSTREAM_HEADER_SIZE);
    dst[8] = p->type; dst[9] = p->format;
    put16(dst + 10, p->flags);
    put32(dst + 12, p->stream_id);
    put64(dst + 16, p->sequence);
    put64(dst + 24, p->sample_index);
    put64(dst + 32, p->timestamp_ns);
    put64(dst + 40, p->center_frequency_hz);
    put32(dst + 48, p->sample_rate_num);
    put32(dst + 52, p->sample_rate_den);
    put32(dst + 56, p->payload_len);
    if (p->payload_len) { memmove(dst + ORCSTREAM_HEADER_SIZE, p->payload, p->payload_len); }
    put32(dst + 60, frame_crc(dst, dst + ORCSTREAM_HEADER_SIZE, p->payload_len));
    *written = full_size;
    return ORCSTREAM_OK;
}

/* Prevalidate header without reading past the first 64 bytes. */
static orcstream_status parse_header(const uint8_t *src, orcstream_packet *out) {
    if (src[0] != 'O' || src[1] != 'R' || src[2] != 'C' || src[3] != 'S') {
        return ORCSTREAM_BAD_MAGIC;
    }
    if (src[4] != ORCSTREAM_VERSION_MAJOR || src[5] != ORCSTREAM_VERSION_MINOR) {
        return ORCSTREAM_BAD_VERSION;
    }
    if (get16(src + 6) != ORCSTREAM_HEADER_SIZE) { return ORCSTREAM_BAD_HEADER; }
    out->type = src[8]; out->format = src[9];
    out->flags = get16(src + 10);
    out->stream_id = get32(src + 12);
    out->sequence = get64(src + 16);
    out->sample_index = get64(src + 24);
    out->timestamp_ns = get64(src + 32);
    out->center_frequency_hz = get64(src + 40);
    out->sample_rate_num = get32(src + 48);
    out->sample_rate_den = get32(src + 52);
    out->payload_len = get32(src + 56);
    out->payload = src + ORCSTREAM_HEADER_SIZE;
    /* A header can be validated before the payload arrives. */
    return validate(out);
}

orcstream_status orcstream_packet_decode(const uint8_t *src, size_t length,
                                          orcstream_packet *out, size_t *consumed) {
    orcstream_packet temp;
    orcstream_status rc;
    size_t needed;
    if (!src || !out || !consumed) { return ORCSTREAM_BAD_ARGUMENT; }
    *consumed = 0;
    if (length < ORCSTREAM_HEADER_SIZE) { return ORCSTREAM_NEED_MORE; }
    rc = parse_header(src, &temp);
    if (rc != ORCSTREAM_OK) { return rc; }
    needed = ORCSTREAM_HEADER_SIZE + (size_t)temp.payload_len;
    if (length < needed) { return ORCSTREAM_NEED_MORE; }
    if (get32(src + 60) != frame_crc(src, temp.payload, temp.payload_len)) {
        return ORCSTREAM_BAD_CHECKSUM;
    }
    if (temp.type == ORCSTREAM_PACKET_HELLO) {
        orcstream_hello hello;
        if (orcstream_hello_decode(temp.payload, temp.payload_len, &hello) != ORCSTREAM_OK) {
            return ORCSTREAM_BAD_HEADER;
        }
    }
    *out = temp;
    *consumed = needed;
    return ORCSTREAM_OK;
}

orcstream_status orcstream_datagram_decode(const uint8_t *src, size_t length,
                                            orcstream_packet *out) {
    size_t used;
    orcstream_packet temp;
    orcstream_status rc = orcstream_packet_decode(src, length, &temp, &used);
    if (rc != ORCSTREAM_OK) { return rc == ORCSTREAM_NEED_MORE ? ORCSTREAM_BAD_LENGTH : rc; }
    if (used != length) { return ORCSTREAM_BAD_LENGTH; }
    *out = temp;
    return ORCSTREAM_OK;
}

orcstream_status orcstream_hello_encode(uint8_t dst[ORCSTREAM_HELLO_SIZE],
                                         const orcstream_hello *hello) {
    if (!dst || !hello) { return ORCSTREAM_BAD_ARGUMENT; }
    if (!hello->max_payload || hello->max_payload > ORCSTREAM_MAX_PAYLOAD ||
        !hello->max_streams || !(hello->capabilities & ORCSTREAM_CAP_IQ_RX) ||
        (hello->capabilities & ~ORCSTREAM_CAP_IQ_RX) ||
        (hello->format_mask & ~0x7eu) || !(hello->format_mask & 0x7eu)) {
        return ORCSTREAM_BAD_HEADER;
    }
    put32(dst + 0, hello->capabilities);
    put32(dst + 4, hello->format_mask);
    put32(dst + 8, hello->max_payload);
    put32(dst + 12, hello->max_streams);
    return ORCSTREAM_OK;
}

orcstream_status orcstream_hello_decode(const uint8_t *src, size_t length,
                                         orcstream_hello *out) {
    orcstream_hello temp;
    uint8_t bytes[ORCSTREAM_HELLO_SIZE];
    if (!src || !out) { return ORCSTREAM_BAD_ARGUMENT; }
    if (length != ORCSTREAM_HELLO_SIZE) { return ORCSTREAM_BAD_LENGTH; }
    temp.capabilities = get32(src + 0);
    temp.format_mask = get32(src + 4);
    temp.max_payload = get32(src + 8);
    temp.max_streams = get32(src + 12);
    if (orcstream_hello_encode(bytes, &temp) != ORCSTREAM_OK) {
        return ORCSTREAM_BAD_HEADER;
    }
    *out = temp;
    return ORCSTREAM_OK;
}

orcstream_status orcstream_reader_init(orcstream_reader *r,
                                        uint8_t *storage, size_t capacity) {
    if (!r || !storage || capacity < ORCSTREAM_HEADER_SIZE ||
        capacity > ORCSTREAM_MAX_FRAME) { return ORCSTREAM_BAD_ARGUMENT; }
    r->storage = storage; r->capacity = capacity; r->used = 0;
    r->target = ORCSTREAM_HEADER_SIZE;
    return ORCSTREAM_OK;
}
void orcstream_reader_reset(orcstream_reader *r) {
    if (r) { r->used = 0; r->target = ORCSTREAM_HEADER_SIZE; }
}
orcstream_status orcstream_reader_feed(orcstream_reader *r,
                                        const uint8_t *input, size_t length,
                                        size_t *consumed, orcstream_packet *out) {
    orcstream_packet temp;
    orcstream_status rc;
    size_t copy_n, parsed;
    if (!r || !r->storage || !consumed || !out || (length && !input) ||
        r->capacity < ORCSTREAM_HEADER_SIZE || r->used > r->target ||
        r->target > r->capacity) { return ORCSTREAM_BAD_ARGUMENT; }
    *consumed = 0;
    if (!length) { return ORCSTREAM_NEED_MORE; }
    copy_n = r->target - r->used;
    if (copy_n > length) { copy_n = length; }
    memcpy(r->storage + r->used, input, copy_n);
    r->used += copy_n;
    *consumed = copy_n;
    if (r->used < r->target) { return ORCSTREAM_NEED_MORE; }
    if (r->target == ORCSTREAM_HEADER_SIZE) {
        rc = parse_header(r->storage, &temp);
        if (rc != ORCSTREAM_OK) { orcstream_reader_reset(r); return rc; }
        r->target = ORCSTREAM_HEADER_SIZE + (size_t)temp.payload_len;
        if (r->target > r->capacity) {
            orcstream_reader_reset(r);
            return ORCSTREAM_NO_SPACE;
        }
        if (r->used < r->target) { return ORCSTREAM_NEED_MORE; }
    }
    rc = orcstream_packet_decode(r->storage, r->target, out, &parsed);
    orcstream_reader_reset(r);
    return rc;
}

void orcstream_tracker_reset(orcstream_tracker *t) {
    if (t) { memset(t, 0, sizeof(*t)); }
}
static void sat_add(uint64_t *value, uint64_t amount) {
    if (UINT64_MAX - *value < amount) { *value = UINT64_MAX; }
    else { *value += amount; }
}
orcstream_status orcstream_tracker_observe(orcstream_tracker *t,
                                            const orcstream_packet *p,
                                            orcstream_continuity *result) {
    size_t bytes;
    uint64_t samples, ending;
    if (!t || !p || !result) { return ORCSTREAM_BAD_ARGUMENT; }
    if (validate(p) != ORCSTREAM_OK || p->type != ORCSTREAM_PACKET_IQ) {
        return ORCSTREAM_BAD_ARGUMENT;
    }
    bytes = orcstream_complex_sample_bytes(p->format);
    samples = p->payload_len / bytes;
    ending = p->sample_index + samples;
    if (t->initialized && t->stream_id != p->stream_id) { return ORCSTREAM_BAD_ARGUMENT; }
    if (!t->initialized) {
        t->initialized = 1;
        t->stream_id = p->stream_id;
        t->next_sequence = p->sequence + 1;
        t->next_sample_index = ending;
        *result = ORCSTREAM_CONTINUITY_FIRST;
        if (p->flags & ORCSTREAM_FLAG_DISCONTINUITY) {
            sat_add(&t->declared_discontinuities, 1);
            *result = ORCSTREAM_CONTINUITY_DECLARED_DISCONTINUITY;
        }
        return ORCSTREAM_OK;
    }
    if (p->flags & ORCSTREAM_FLAG_DISCONTINUITY) {
        sat_add(&t->declared_discontinuities, 1);
        t->next_sequence = p->sequence + 1;
        t->next_sample_index = ending;
        *result = ORCSTREAM_CONTINUITY_DECLARED_DISCONTINUITY;
        return ORCSTREAM_OK;
    }
    if (p->sequence < t->next_sequence || p->sample_index < t->next_sample_index) {
        sat_add(&t->late_or_duplicate, 1);
        *result = ORCSTREAM_CONTINUITY_LATE_OR_DUPLICATE;
        return ORCSTREAM_OK;
    }
    *result = ORCSTREAM_CONTINUITY_OK;
    if (p->sequence > t->next_sequence) {
        sat_add(&t->missing_packets, p->sequence - t->next_sequence);
        *result = ORCSTREAM_CONTINUITY_GAP;
    }
    if (p->sample_index > t->next_sample_index) {
        sat_add(&t->missing_samples, p->sample_index - t->next_sample_index);
        *result = ORCSTREAM_CONTINUITY_GAP;
    }
    t->next_sequence = p->sequence + 1;
    t->next_sample_index = ending;
    return ORCSTREAM_OK;
}

const char *orcstream_status_string(orcstream_status rc) {
    switch (rc) {
        case ORCSTREAM_OK: return "ok";
        case ORCSTREAM_NEED_MORE: return "need more data";
        case ORCSTREAM_BAD_ARGUMENT: return "bad argument";
        case ORCSTREAM_NO_SPACE: return "buffer too small";
        case ORCSTREAM_BAD_MAGIC: return "invalid magic";
        case ORCSTREAM_BAD_VERSION: return "unsupported version";
        case ORCSTREAM_BAD_HEADER: return "invalid header";
        case ORCSTREAM_BAD_TYPE: return "unsupported packet type";
        case ORCSTREAM_BAD_FORMAT: return "unsupported sample format";
        case ORCSTREAM_BAD_FLAGS: return "invalid flags";
        case ORCSTREAM_BAD_LENGTH: return "invalid length";
        case ORCSTREAM_BAD_CHECKSUM: return "checksum mismatch";
        default: return "unknown status";
    }
}
