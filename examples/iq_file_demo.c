/* Portable, offline CU8 IQ roundtrip: proof the SDK does not require TCP. */
/* SPDX-License-Identifier: MIT */
#include "orcstream/orcstream.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLES_PER_FRAME 256u
#define FRAME_COUNT_LIMIT 100000u
#define FILE_READER_BYTES 4096u

static void init_iq(orcstream_packet *p, const uint8_t *samples) {
    memset(p, 0, sizeof(*p));
    p->type = ORCSTREAM_PACKET_IQ;
    p->format = ORCSTREAM_FORMAT_CU8;
    p->flags = ORCSTREAM_FLAG_FREQUENCY_VALID;
    p->stream_id = 1;
    p->sample_rate_num = 2400000;
    p->sample_rate_den = 1;
    p->center_frequency_hz = 100000000;
    p->payload = samples;
    p->payload_len = SAMPLES_PER_FRAME * 2;
}
static void fill_samples(uint8_t *samples, uint64_t start) {
    size_t i;
    for (i = 0; i < SAMPLES_PER_FRAME; ++i) {
        uint64_t idx = start + i;
        samples[i * 2] = (uint8_t)(idx & 0xffu);
        samples[i * 2 + 1] = (uint8_t)((idx * 3u + 127u) & 0xffu);
    }
}
static int write_capture(const char *path, unsigned count) {
    uint8_t payload[SAMPLES_PER_FRAME * 2], frame[ORCSTREAM_HEADER_SIZE + sizeof(payload)];
    uint8_t hello[ORCSTREAM_HELLO_SIZE];
    orcstream_hello options = { ORCSTREAM_CAP_IQ_RX, 1u << ORCSTREAM_FORMAT_CU8,
                                1152, 1 };
    orcstream_packet p;
    FILE *f;
    size_t written;
    unsigned i;
    if (!count || count > FRAME_COUNT_LIMIT) { return 1; }
    f = fopen(path, "wb");
    if (!f) { perror("open"); return 1; }
    if (orcstream_hello_encode(hello, &options) != ORCSTREAM_OK) { fclose(f); return 1; }
    memset(&p, 0, sizeof(p));
    p.type = ORCSTREAM_PACKET_HELLO;
    p.payload = hello;
    p.payload_len = sizeof(hello);
    if (orcstream_packet_encode(frame, sizeof(frame), &p, &written) != ORCSTREAM_OK ||
        fwrite(frame, 1, written, f) != written) { fclose(f); return 1; }
    init_iq(&p, payload);
    for (i = 0; i < count; ++i) {
        p.sequence = i;
        p.sample_index = (uint64_t)i * SAMPLES_PER_FRAME;
        fill_samples(payload, p.sample_index);
        if (orcstream_packet_encode(frame, sizeof(frame), &p, &written) != ORCSTREAM_OK ||
            fwrite(frame, 1, written, f) != written) { fclose(f); return 1; }
    }
    if (fclose(f)) { return 1; }
    printf("Wrote %u CU8 IQ packets, %llu complex samples to %s\n",
           count, (unsigned long long)count * SAMPLES_PER_FRAME, path);
    return 0;
}
static int inspect(const orcstream_packet *p, orcstream_tracker *tracker,
                   unsigned long *packets, int *saw_hello) {
    orcstream_continuity continuity;
    uint8_t expected[SAMPLES_PER_FRAME * 2];
    orcstream_hello hello;
    if (p->type == ORCSTREAM_PACKET_HELLO) {
        if (*saw_hello || *packets ||
            orcstream_hello_decode(p->payload, p->payload_len, &hello) != ORCSTREAM_OK ||
            hello.format_mask != (1u << ORCSTREAM_FORMAT_CU8)) { return 1; }
        *saw_hello = 1;
        return 0;
    }
    if (!*saw_hello || p->type != ORCSTREAM_PACKET_IQ ||
        p->format != ORCSTREAM_FORMAT_CU8 ||
        p->payload_len != SAMPLES_PER_FRAME * 2 ||
        p->sequence != *packets ||
        p->sample_index != (uint64_t)*packets * SAMPLES_PER_FRAME) { return 1; }
    fill_samples(expected, p->sample_index);
    if (memcmp(expected, p->payload, sizeof(expected)) ||
        orcstream_tracker_observe(tracker, p, &continuity) != ORCSTREAM_OK ||
        (continuity != ORCSTREAM_CONTINUITY_FIRST && continuity != ORCSTREAM_CONTINUITY_OK)) {
        return 1;
    }
    (*packets)++;
    return 0;
}
static int verify_capture(const char *path) {
    uint8_t scratch[FILE_READER_BYTES], input[37];
    orcstream_reader r;
    orcstream_packet p;
    orcstream_tracker tracker;
    unsigned long packets = 0;
    int saw_hello = 0;
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f) { perror("open"); return 1; }
    orcstream_tracker_reset(&tracker);
    if (orcstream_reader_init(&r, scratch, sizeof(scratch)) != ORCSTREAM_OK) {
        fclose(f); return 1;
    }
    while ((n = fread(input, 1, sizeof(input), f)) > 0) {
        size_t offset = 0;
        while (offset < n) {
            size_t used = 0;
            orcstream_status rc = orcstream_reader_feed(&r, input + offset,
                                                        n - offset, &used, &p);
            if (rc != ORCSTREAM_OK && rc != ORCSTREAM_NEED_MORE) {
                fprintf(stderr, "Packet decode failed: %s\n", orcstream_status_string(rc));
                fclose(f); return 1;
            }
            offset += used;
            if (rc == ORCSTREAM_OK && inspect(&p, &tracker, &packets, &saw_hello)) {
                fprintf(stderr, "IQ sample integrity or sequence mismatch\n");
                fclose(f); return 1;
            }
            if (used == 0) { fclose(f); return 1; }
        }
    }
    if (ferror(f) || r.used != 0 || !saw_hello || !packets) {
        fprintf(stderr, "Truncated or empty stream\n"); fclose(f); return 1;
    }
    fclose(f);
    printf("Verified %lu IQ packets; samples=%llu, missing_packets=%llu, missing_samples=%llu\n",
           packets, (unsigned long long)packets * SAMPLES_PER_FRAME,
           (unsigned long long)tracker.missing_packets,
           (unsigned long long)tracker.missing_samples);
    return 0;
}
int main(int argc, char **argv) {
    if (argc >= 3 && strcmp(argv[1], "write") == 0) {
        unsigned count = 48;
        if (argc == 4) {
            char *end;
            unsigned long v = strtoul(argv[3], &end, 10);
            if (!argv[3][0] || *end || v > FRAME_COUNT_LIMIT || !v) {
                fprintf(stderr, "Invalid frame count\n"); return 2;
            }
            count = (unsigned)v;
        }
        return write_capture(argv[2], count);
    }
    if (argc == 3 && strcmp(argv[1], "verify") == 0) {
        return verify_capture(argv[2]);
    }
    fprintf(stderr, "Usage: %s write <file.orcs> [frames]\n"
                    "       %s verify <file.orcs>\n", argv[0], argv[0]);
    return 2;
}
