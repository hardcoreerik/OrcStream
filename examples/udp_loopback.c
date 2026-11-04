/* OrcStream UDP reference adapter: synthetic full-frame CU8 IQ, IPv4 only.
 * No authentication, encryption, routing, control, or congestion handling.
 * Use only on a trusted local test network. SPDX-License-Identifier: MIT */
#include "orcstream/orcstream.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET os_socket;
#define INVALID_OS_SOCKET INVALID_SOCKET
#define socket_close closesocket
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
typedef int os_socket;
#define INVALID_OS_SOCKET (-1)
#define socket_close close
#endif

#define DATA_COMPLEX_SAMPLES 256u
#define DATA_BYTES (DATA_COMPLEX_SAMPLES * 2u)
#define FRAME_BYTES (ORCSTREAM_HEADER_SIZE + DATA_BYTES)
#define MAX_EXAMPLE_PACKETS 100000u

static void fill_iq(uint8_t *buf, uint64_t sample_index) {
    unsigned i;
    for (i = 0; i < DATA_COMPLEX_SAMPLES; ++i) {
        buf[2*i] = (uint8_t)((sample_index + i) & 0xffu);
        buf[2*i+1] = (uint8_t)(((sample_index + i) * 3u + 127u) & 0xffu);
    }
}
static int net_init(void) {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2,2), &wsa) == 0;
#else
    return 1;
#endif
}
static void net_finish(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}
static int parse_num(const char *s, unsigned *out, unsigned min, unsigned max) {
    unsigned long n;
    char *end;
    if (!s || !*s) { return 0; }
    n = strtoul(s, &end, 10);
    if (*end || n < min || n > max) { return 0; }
    *out = (unsigned)n;
    return 1;
}
static int emit(os_socket fd, const struct sockaddr_in *dest,
                const uint8_t *buffer, size_t len) {
    int sent = sendto(fd, (const char *)buffer, (int)len, 0,
                      (const struct sockaddr *)dest, (int)sizeof(*dest));
    return sent == (int)len;
}
static int sender(const char *address, unsigned port, unsigned packets) {
    os_socket fd;
    struct sockaddr_in dest;
    uint8_t data[DATA_BYTES], frame[FRAME_BYTES], hello[ORCSTREAM_HELLO_SIZE];
    size_t written;
    unsigned i;
    orcstream_packet p;
    orcstream_hello options = { ORCSTREAM_CAP_IQ_RX, 1u << ORCSTREAM_FORMAT_CU8,
                                1152, 1 };
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == INVALID_OS_SOCKET) { return 1; }
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, address, &dest.sin_addr) != 1) {
        fprintf(stderr, "Use an IPv4 address such as 127.0.0.1\n");
        socket_close(fd); return 1;
    }
    if (orcstream_hello_encode(hello, &options) != ORCSTREAM_OK) {
        socket_close(fd); return 1;
    }
    memset(&p, 0, sizeof(p));
    p.type = ORCSTREAM_PACKET_HELLO;
    p.payload = hello; p.payload_len = sizeof(hello);
    if (orcstream_packet_encode(frame, sizeof(frame), &p, &written) != ORCSTREAM_OK ||
        !emit(fd, &dest, frame, written)) { socket_close(fd); return 1; }
    memset(&p, 0, sizeof(p));
    p.type = ORCSTREAM_PACKET_IQ;
    p.format = ORCSTREAM_FORMAT_CU8;
    p.flags = ORCSTREAM_FLAG_FREQUENCY_VALID;
    p.center_frequency_hz = 100000000;
    p.stream_id = 1;
    p.sample_rate_num = 2400000;
    p.sample_rate_den = 1;
    p.payload = data; p.payload_len = sizeof(data);
    for (i = 0; i < packets; ++i) {
        p.sequence = i;
        p.sample_index = (uint64_t)i * DATA_COMPLEX_SAMPLES;
        fill_iq(data, p.sample_index);
        if (orcstream_packet_encode(frame, sizeof(frame), &p, &written) != ORCSTREAM_OK ||
            !emit(fd, &dest, frame, written)) {
            socket_close(fd); return 1;
        }
    }
    socket_close(fd);
    printf("UDP sender transmitted HELLO + %u IQ packets to %s:%u\n", packets, address, port);
    return 0;
}
static int receiver(unsigned port, unsigned expected_packets) {
    os_socket fd;
    struct sockaddr_in bind_addr;
    uint8_t frame[1500], expected[DATA_BYTES];
    orcstream_packet p;
    orcstream_tracker tracker;
    unsigned received = 0, got_hello = 0;
    int rc = 1;
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == INVALID_OS_SOCKET) { return 1; }
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* intentionally local only */
    bind_addr.sin_port = htons((uint16_t)port);
    if (bind(fd, (const struct sockaddr *)&bind_addr, sizeof(bind_addr)) != 0) {
        fprintf(stderr, "Could not bind receiver\n"); socket_close(fd); return 1;
    }
#ifdef _WIN32
    {
        DWORD timeout = 4000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    }
#else
    {
        struct timeval timeout;
        timeout.tv_sec = 4;
        timeout.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }
#endif
    orcstream_tracker_reset(&tracker);
    while (received < expected_packets) {
        int count = recvfrom(fd, (char *)frame, sizeof(frame), 0, NULL, NULL);
        orcstream_continuity continuity;
        if (count <= 0) { fprintf(stderr, "UDP receive timed out or failed\n"); break; }
        if (orcstream_datagram_decode(frame, (size_t)count, &p) != ORCSTREAM_OK) {
            fprintf(stderr, "Invalid or corrupted UDP frame\n"); break;
        }
        if (p.type == ORCSTREAM_PACKET_HELLO) {
            orcstream_hello h;
            if (got_hello || received || orcstream_hello_decode(p.payload, p.payload_len, &h) != ORCSTREAM_OK ||
                !(h.format_mask & (1u << ORCSTREAM_FORMAT_CU8))) { break; }
            got_hello = 1;
            continue;
        }
        if (!got_hello || p.type != ORCSTREAM_PACKET_IQ ||
            p.format != ORCSTREAM_FORMAT_CU8 || p.payload_len != DATA_BYTES) { break; }
        fill_iq(expected, p.sample_index);
        if (memcmp(expected, p.payload, DATA_BYTES) != 0 ||
            orcstream_tracker_observe(&tracker, &p, &continuity) != ORCSTREAM_OK ||
            continuity == ORCSTREAM_CONTINUITY_LATE_OR_DUPLICATE) { break; }
        received++;
    }
    if (got_hello && received == expected_packets) {
        printf("UDP receiver verified %u IQ packets, missing_packets=%llu, missing_samples=%llu\n",
               received, (unsigned long long)tracker.missing_packets,
               (unsigned long long)tracker.missing_samples);
        rc = tracker.missing_packets || tracker.missing_samples ? 1 : 0;
    }
    socket_close(fd);
    return rc;
}
int main(int argc, char **argv) {
    unsigned port, packets = 48;
    int rc;
    if (argc < 3 || argc > 5) { goto usage; }
    if (strcmp(argv[1], "receive") == 0) {
        if (argc > 4 || !parse_num(argv[2], &port, 1, 65535)) { goto usage; }
        if (argc == 4 && !parse_num(argv[3], &packets, 1, MAX_EXAMPLE_PACKETS)) { goto usage; }
        if (!net_init()) { return 1; }
        rc = receiver(port, packets);
        net_finish(); return rc;
    }
    if (strcmp(argv[1], "send") == 0) {
        if (argc < 4 || !parse_num(argv[3], &port, 1, 65535)) { goto usage; }
        if (argc == 5 && !parse_num(argv[4], &packets, 1, MAX_EXAMPLE_PACKETS)) { goto usage; }
        if (!net_init()) { return 1; }
        rc = sender(argv[2], port, packets);
        net_finish(); return rc;
    }
usage:
    fprintf(stderr, "Usage: %s receive <port> [frames]\n"
                    "       %s send <ipv4> <port> [frames]\n", argv[0], argv[0]);
    return 2;
}
