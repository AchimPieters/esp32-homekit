#pragma once

#include <stdint.h>
#include <string.h>

// Pure DNS name-parsing helpers shared by the (ESP-OPEN-RTOS / host) mDNS
// responder. Kept header-only and free of platform dependencies so they can be
// unit-tested on the host. All functions take the raw packet (data/size) plus a
// byte offset and return the offset just past the matched/skipped name, or a
// negative value on malformed input.

// Maximum number of DNS name-compression pointers to follow before giving up.
// Guards against an infinite loop on a packet with cyclic compression pointers.
#ifndef MDNS_MAX_COMPRESSION_POINTERS
#define MDNS_MAX_COMPRESSION_POINTERS 8
#endif

static inline int mdns_match_label(uint8_t *data, uint16_t size, uint16_t offset, const char *name, uint8_t len) {
        int pointers = 0;
        while (offset + 1 < size && (data[offset] & 0xC0) == 0xC0) {
                if (++pointers > MDNS_MAX_COMPRESSION_POINTERS)
                        return -1;
                offset = ((data[offset] & 0x3F) << 8) + data[offset+1];
                if (offset >= size)
                        return -1;
        }

        if (offset + 1 + len > size)
                return -1;

        if (data[offset] != len)
                return -1;

        if (len && memcmp(data + 1 + offset, name, len))
                return -1;

        return offset + 1 + len;
}

static inline int mdns_skip_name(uint8_t *data, uint16_t size, uint16_t offset) {
        while (offset < size && data[offset] && !(data[offset] & 0xC0)) {
                offset += 1 + data[offset];
        }

        if (offset >= size)
                return -1;

        if ((data[offset] & 0xC0) == 0xC0)
                return offset + 2;

        return offset + 1;
}
