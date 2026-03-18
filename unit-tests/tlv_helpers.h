#pragma once

#include <stdint.h>
#include <string.h>

/**
 * @brief TLV encoding helpers for unit tests.
 *
 * The TLV format follows DER encoding rules:
 *  - Tags/lengths < 0x80 are encoded as a single byte.
 *  - Tags/lengths >= 0x80 use long form: [0x80 | byte_count, value_bytes...].
 *
 * Integer values are encoded in big-endian with minimal bytes (no leading zeros).
 */

/**
 * DER-encodes a tag or length value into buf; returns the number of bytes written.
 */
static inline size_t der_encode(uint8_t *buf, uint32_t value) {
    if (value < 0x80) {
        buf[0] = (uint8_t) value;
        return 1;
    } else if (value <= 0xFF) {
        buf[0] = 0x81;
        buf[1] = (uint8_t) value;
        return 2;
    } else if (value <= 0xFFFF) {
        buf[0] = 0x82;
        buf[1] = (uint8_t) (value >> 8);
        buf[2] = (uint8_t) value;
        return 3;
    } else {
        buf[0] = 0x83;
        buf[1] = (uint8_t) (value >> 16);
        buf[2] = (uint8_t) (value >> 8);
        buf[3] = (uint8_t) value;
        return 4;
    }
}

/**
 * Encodes an integer value as minimal big-endian bytes into buf.
 * Returns the number of bytes written (1 minimum).
 */
static inline size_t encode_int_be(uint8_t *buf, uint64_t value) {
    uint8_t tmp[8];
    size_t  len = 0;

    tmp[7] = (uint8_t) value;
    tmp[6] = (uint8_t) (value >> 8);
    tmp[5] = (uint8_t) (value >> 16);
    tmp[4] = (uint8_t) (value >> 24);
    tmp[3] = (uint8_t) (value >> 32);
    tmp[2] = (uint8_t) (value >> 40);
    tmp[1] = (uint8_t) (value >> 48);
    tmp[0] = (uint8_t) (value >> 56);

    /* find first non-zero byte */
    size_t start = 0;
    while (start < 7 && tmp[start] == 0) {
        start++;
    }
    len = 8 - start;
    memcpy(buf, &tmp[start], len);
    return len;
}

/**
 * Appends a TLV field to buf at *offset. Updates *offset.
 *
 * @param buf     Output buffer. The caller is responsible for ensuring buf is
 *                large enough to hold the encoded tag, length, and value bytes.
 *                No bounds checking is performed; an undersized buffer will
 *                result in out-of-bounds writes and undefined behaviour.
 * @param offset  Current write position (updated)
 * @param tag     TLV tag value
 * @param value   Pointer to raw value bytes
 * @param vlen    Length of value bytes
 */
static inline void tlv_append_field(uint8_t *buf,
                                    size_t  *offset,
                                    uint32_t tag,
                                    const uint8_t *value,
                                    size_t         vlen) {
    *offset += der_encode(&buf[*offset], tag);
    *offset += der_encode(&buf[*offset], (uint32_t) vlen);
    memcpy(&buf[*offset], value, vlen);
    *offset += vlen;
}

/**
 * Appends an integer TLV field (big-endian, minimal bytes) to buf at *offset.
 */
static inline void tlv_append_uint(uint8_t *buf, size_t *offset, uint32_t tag, uint64_t value) {
    uint8_t vbuf[8];
    size_t  vlen = encode_int_be(vbuf, value);
    tlv_append_field(buf, offset, tag, vbuf, vlen);
}

/**
 * Appends a boolean TLV field to buf at *offset.
 */
static inline void tlv_append_bool(uint8_t *buf, size_t *offset, uint32_t tag, int value) {
    uint8_t v = value ? 1 : 0;
    tlv_append_field(buf, offset, tag, &v, 1);
}

/**
 * Appends a string TLV field to buf at *offset (not NUL-terminated in TLV).
 */
static inline void tlv_append_str(uint8_t    *buf,
                                   size_t     *offset,
                                   uint32_t    tag,
                                   const char *str) {
    size_t len = strlen(str);
    tlv_append_field(buf, offset, tag, (const uint8_t *) str, len);
}

/**
 * Appends a raw bytes TLV field (fixed-length blob) to buf at *offset.
 */
static inline void tlv_append_bytes(uint8_t       *buf,
                                    size_t         *offset,
                                    uint32_t        tag,
                                    const uint8_t  *bytes,
                                    size_t          len) {
    tlv_append_field(buf, offset, tag, bytes, len);
}

/**
 * Appends a signed int64 TLV field as full 8 bytes big-endian to buf at *offset.
 * Use this for ntli which Python encodes via struct.pack(">q").
 */
static inline void tlv_append_int64_be(uint8_t *buf,
                                        size_t  *offset,
                                        uint32_t tag,
                                        int64_t  value) {
    uint8_t vbuf[8];
    uint64_t u;

    memcpy(&u, &value, sizeof(u));
    vbuf[0] = (uint8_t) (u >> 56);
    vbuf[1] = (uint8_t) (u >> 48);
    vbuf[2] = (uint8_t) (u >> 40);
    vbuf[3] = (uint8_t) (u >> 32);
    vbuf[4] = (uint8_t) (u >> 24);
    vbuf[5] = (uint8_t) (u >> 16);
    vbuf[6] = (uint8_t) (u >> 8);
    vbuf[7] = (uint8_t) u;
    tlv_append_field(buf, offset, tag, vbuf, 8);
}

/**
 * Creates a buffer_t pointing at the given array slice.
 */
#include "buffer.h"
static inline buffer_t make_buffer(uint8_t *ptr, size_t size) {
    return (buffer_t){.ptr = ptr, .size = size, .offset = 0};
}
