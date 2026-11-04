/* SPDX-License-Identifier: MIT */
#include "orcstream/orcstream.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #x); return 1; \
} } while(0)

static orcstream_packet sample(uint8_t format, const uint8_t *payload, uint32_t length) {
    orcstream_packet p;
    memset(&p, 0, sizeof(p));
    p.type = ORCSTREAM_PACKET_IQ;
    p.format = format;
    p.stream_id = 0x12345678u;
    p.sequence = 0x0102030405060708ull;
    p.sample_index = 144;
    p.sample_rate_num = 2400000u;
    p.sample_rate_den = 1;
    p.flags = ORCSTREAM_FLAG_TIMESTAMP_VALID | ORCSTREAM_FLAG_CLOCK_UTC |
              ORCSTREAM_FLAG_FREQUENCY_VALID;
    p.timestamp_ns = 0x1122334455667788ull;
    p.center_frequency_hz = 105500000ull;
    p.payload = payload;
    p.payload_len = length;
    return p;
}
static int check_formats(void) {
    const uint8_t formats[] = { ORCSTREAM_FORMAT_CU8, ORCSTREAM_FORMAT_CS8,
         ORCSTREAM_FORMAT_CS16_LE, ORCSTREAM_FORMAT_CS16_BE,
         ORCSTREAM_FORMAT_CF32_LE, ORCSTREAM_FORMAT_CF32_BE };
    const uint8_t data[16] = { 0, 128, 255, 1, 10, 20, 30, 40,
                               0x55, 0xaa, 0x41, 0x80, 0, 0, 0, 0 };
    uint8_t buf[256];
    size_t i;
    CHECK(orcstream_complex_sample_bytes(0) == 0);
    CHECK(orcstream_complex_sample_bytes(7) == 0);
    for (i = 0; i < sizeof(formats); ++i) {
        orcstream_packet p = sample(formats[i], data, 16);
        orcstream_packet q;
        size_t out = 0, consumed = 0;
        CHECK(orcstream_packet_encode(buf, sizeof(buf), &p, &out) == ORCSTREAM_OK);
        CHECK(out == 80);
        CHECK(buf[0] == 'O' && buf[1] == 'R' && buf[2] == 'C' && buf[3] == 'S');
        CHECK(buf[4] == 0 && buf[5] == 1 && buf[6] == 0 && buf[7] == 64);
        CHECK(buf[12] == 0x12 && buf[13] == 0x34 && buf[14] == 0x56 && buf[15] == 0x78);
        CHECK(buf[16] == 1 && buf[23] == 8);
        CHECK(buf[32] == 0x11 && buf[39] == 0x88);
        CHECK(buf[56] == 0 && buf[57] == 0 && buf[58] == 0 && buf[59] == 16);
        CHECK(orcstream_packet_decode(buf, 79, &q, &consumed) == ORCSTREAM_NEED_MORE);
        CHECK(orcstream_packet_decode(buf, 80, &q, &consumed) == ORCSTREAM_OK);
        CHECK(consumed == 80 && q.payload_len == 16 && q.format == formats[i]);
        CHECK(q.sequence == p.sequence && q.stream_id == p.stream_id);
        CHECK(q.timestamp_ns == p.timestamp_ns && q.center_frequency_hz == p.center_frequency_hz);
        CHECK(q.sample_index == p.sample_index && q.sample_rate_num == p.sample_rate_num);
        CHECK(memcmp(q.payload, data, 16) == 0);
        CHECK(orcstream_datagram_decode(buf, 80, &q) == ORCSTREAM_OK);
        CHECK(orcstream_datagram_decode(buf, 81, &q) == ORCSTREAM_BAD_LENGTH);
        buf[67] ^= 0x01;
        CHECK(orcstream_packet_decode(buf, 80, &q, &consumed) == ORCSTREAM_BAD_CHECKSUM);
        buf[67] ^= 0x01;
        buf[0] ^= 0x01;
        CHECK(orcstream_packet_decode(buf, 80, &q, &consumed) == ORCSTREAM_BAD_MAGIC);
        buf[0] ^= 0x01;
        buf[5] ^= 0x01;
        CHECK(orcstream_packet_decode(buf, 80, &q, &consumed) == ORCSTREAM_BAD_VERSION);
        buf[5] ^= 0x01;
        buf[7] = 63;
        CHECK(orcstream_packet_decode(buf, 80, &q, &consumed) == ORCSTREAM_BAD_HEADER);
        buf[7] = 64;
        buf[9] = 0;
        CHECK(orcstream_packet_decode(buf, 80, &q, &consumed) == ORCSTREAM_BAD_FORMAT);
        buf[9] = formats[i];
        buf[10] = 0x80;
        CHECK(orcstream_packet_decode(buf, 80, &q, &consumed) == ORCSTREAM_BAD_FLAGS);
    }
    return 0;
}
static int check_invalid(void) {
    uint8_t data[16] = {0};
    uint8_t buf[128];
    orcstream_packet p = sample(ORCSTREAM_FORMAT_CU8, data, 16), out;
    size_t size, consumed;
    CHECK(orcstream_packet_encode(buf, 79, &p, &size) == ORCSTREAM_NO_SPACE);
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_OK);
    buf[56] = 0xff; buf[57] = 0xff; buf[58] = 0xff; buf[59] = 0xff;
    CHECK(orcstream_packet_decode(buf, 80, &out, &consumed) == ORCSTREAM_BAD_LENGTH);
    p.payload_len = 17;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_LENGTH);
    p.payload_len = 16;
    p.flags = ORCSTREAM_FLAG_TIMESTAMP_VALID;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_FLAGS);
    p.flags = ORCSTREAM_FLAG_CLOCK_UTC | ORCSTREAM_FLAG_FREQUENCY_VALID;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_FLAGS);
    p.flags = ORCSTREAM_FLAG_FREQUENCY_VALID;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_FLAGS);
    p.timestamp_ns = 0;
    p.flags = 0;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_FLAGS);
    p.center_frequency_hz = 0;
    p.sample_rate_den = 0;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_HEADER);
    p.sample_rate_den = 1;
    p.stream_id = 0;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_HEADER);
    p.stream_id = 1;
    p.type = 99;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_TYPE);
    p.type = ORCSTREAM_PACKET_IQ;
    p.sample_index = UINT64_MAX - 7;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_HEADER);
    p.sample_index = 0;
    p.payload_len = 0;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_BAD_LENGTH);
    p.flags = ORCSTREAM_FLAG_END_OF_STREAM;
    CHECK(orcstream_packet_encode(buf, 128, &p, &size) == ORCSTREAM_OK);
    CHECK(size == 64);
    CHECK(orcstream_datagram_decode(buf, size, &out) == ORCSTREAM_OK);
    CHECK(orcstream_packet_decode(NULL, 0, &out, &consumed) == ORCSTREAM_BAD_ARGUMENT);
    CHECK(strcmp(orcstream_status_string(ORCSTREAM_BAD_CHECKSUM), "checksum mismatch") == 0);
    return 0;
}
static int check_hello(void) {
    uint8_t data[ORCSTREAM_HELLO_SIZE];
    uint8_t bytes[128];
    orcstream_hello h = { ORCSTREAM_CAP_IQ_RX,
        (1u << ORCSTREAM_FORMAT_CU8) | (1u << ORCSTREAM_FORMAT_CS16_LE), 1152, 1 };
    orcstream_hello got;
    orcstream_packet p, q;
    size_t written = 0, used = 0;
    CHECK(orcstream_hello_encode(data, &h) == ORCSTREAM_OK);
    CHECK(orcstream_hello_decode(data, 16, &got) == ORCSTREAM_OK);
    CHECK(got.max_payload == 1152 && got.format_mask == h.format_mask);
    CHECK(orcstream_hello_decode(data, 15, &got) == ORCSTREAM_BAD_LENGTH);
    data[0] = 0x80;
    CHECK(orcstream_hello_decode(data, 16, &got) == ORCSTREAM_BAD_HEADER);
    data[0] = 0;
    memset(&p, 0, sizeof(p));
    p.type = ORCSTREAM_PACKET_HELLO; p.payload = data; p.payload_len = 16;
    CHECK(orcstream_packet_encode(bytes, sizeof(bytes), &p, &written) == ORCSTREAM_OK);
    CHECK(written == 80);
    CHECK(orcstream_packet_decode(bytes, written, &q, &used) == ORCSTREAM_OK);
    CHECK(orcstream_hello_decode(q.payload, q.payload_len, &got) == ORCSTREAM_OK);
    p.stream_id = 2;
    CHECK(orcstream_packet_encode(bytes, sizeof(bytes), &p, &written) == ORCSTREAM_BAD_HEADER);
    return 0;
}
static int check_reader(void) {
    uint8_t data[12] = {0, 128, 1, 127, 2, 126, 3, 125, 4, 124, 5, 123};
    uint8_t pair[152], storage[128];
    orcstream_packet p = sample(ORCSTREAM_FORMAT_CU8, data, 12), q;
    orcstream_reader reader;
    size_t written, offset = 0, count = 0;
    CHECK(orcstream_packet_encode(pair, sizeof(pair), &p, &written) == ORCSTREAM_OK);
    CHECK(written == 76);
    p.sequence++;
    p.sample_index += 6;
    CHECK(orcstream_packet_encode(pair + written, sizeof(pair) - written, &p, &written) == ORCSTREAM_OK);
    CHECK(orcstream_reader_init(&reader, storage, sizeof(storage)) == ORCSTREAM_OK);
    while (offset < sizeof(pair)) {
        size_t used = 0, bite = 1 + (offset % 13);
        orcstream_status rc;
        if (bite > sizeof(pair) - offset) { bite = sizeof(pair) - offset; }
        rc = orcstream_reader_feed(&reader, pair + offset, bite, &used, &q);
        CHECK(rc == ORCSTREAM_OK || rc == ORCSTREAM_NEED_MORE);
        CHECK(used > 0 && used <= bite);
        offset += used;
        if (rc == ORCSTREAM_OK) {
            CHECK(q.sample_index == 144 + 6 * count);
            CHECK(q.payload_len == 12 && memcmp(q.payload, data, 12) == 0);
            count++;
        }
    }
    CHECK(count == 2);
    CHECK(orcstream_reader_init(&reader, storage, 65) == ORCSTREAM_OK);
    {
        size_t used;
        CHECK(orcstream_reader_feed(&reader, pair, 76, &used, &q) == ORCSTREAM_NO_SPACE);
        CHECK(used == 64 && reader.used == 0);
    }
    CHECK(orcstream_reader_init(&reader, storage, 63) == ORCSTREAM_BAD_ARGUMENT);
    return 0;
}
static int check_tracking(void) {
    uint8_t iq[8] = {0};
    orcstream_packet p = sample(ORCSTREAM_FORMAT_CU8, iq, 8);
    orcstream_tracker t;
    orcstream_continuity c;
    orcstream_tracker_reset(&t);
    CHECK(orcstream_tracker_observe(&t, &p, &c) == ORCSTREAM_OK);
    CHECK(c == ORCSTREAM_CONTINUITY_FIRST && t.next_sample_index == 148);
    p.sequence++; p.sample_index += 4;
    CHECK(orcstream_tracker_observe(&t, &p, &c) == ORCSTREAM_OK);
    CHECK(c == ORCSTREAM_CONTINUITY_OK);
    p.sequence += 3; p.sample_index += 12;
    CHECK(orcstream_tracker_observe(&t, &p, &c) == ORCSTREAM_OK);
    CHECK(c == ORCSTREAM_CONTINUITY_GAP);
    CHECK(t.missing_packets == 2 && t.missing_samples == 8);
    CHECK(orcstream_tracker_observe(&t, &p, &c) == ORCSTREAM_OK);
    CHECK(c == ORCSTREAM_CONTINUITY_LATE_OR_DUPLICATE && t.late_or_duplicate == 1);
    p.flags |= ORCSTREAM_FLAG_DISCONTINUITY;
    p.sequence = 0; p.sample_index = 0;
    CHECK(orcstream_tracker_observe(&t, &p, &c) == ORCSTREAM_OK);
    CHECK(c == ORCSTREAM_CONTINUITY_DECLARED_DISCONTINUITY);
    CHECK(t.declared_discontinuities == 1 && t.next_sequence == 1);
    p.stream_id++;
    CHECK(orcstream_tracker_observe(&t, &p, &c) == ORCSTREAM_BAD_ARGUMENT);
    return 0;
}
static int check_fuzz(void) {
    uint8_t data[128];
    orcstream_packet p;
    size_t used, i, j;
    uint32_t seed = 0xa5c31e27u;
    for (i = 0; i < 8000; ++i) {
        for (j = 0; j < sizeof(data); ++j) {
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
            data[j] = (uint8_t)seed;
        }
        (void)orcstream_packet_decode(data, i % sizeof(data), &p, &used);
    }
    return 0;
}
int main(void) {
    if (check_formats() || check_invalid() || check_hello() ||
        check_reader() || check_tracking() || check_fuzz()) { return 1; }
    printf("OrcStream v0.1 core: all checks passed\n");
    return 0;
}
