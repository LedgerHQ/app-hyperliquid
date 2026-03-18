#pragma once
/**
 * Helpers for reading back msgpack output produced by serialize functions.
 * Uses the existing cmp library — no new stubs required.
 *
 * Usage:
 *   ser_buf_t sb = {0};
 *   // ... run serialize function into sb ...
 *   cmp_ctx_t r = make_reader(&sb);
 *   uint32_t map_sz;
 *   assert_true(cmp_read_map(&r, &map_sz));
 *   assert_int_equal(map_sz, 4);
 *   // read each key/value with cmp_read_str / cmp_read_uinteger etc.
 */

#include <string.h>
#include <stddef.h>
#include "cmp.h"

typedef struct {
    uint8_t data[512];
    size_t  len;
    size_t  rpos;
} ser_buf_t;

static size_t ser_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    ser_buf_t *sb = (ser_buf_t *) ctx->buf;
    if (sb->len + count > sizeof(sb->data)) return 0;
    memcpy(&sb->data[sb->len], data, count);
    sb->len += count;
    return count;
}

static bool ser_reader(cmp_ctx_t *ctx, void *data, size_t count) {
    ser_buf_t *sb = (ser_buf_t *) ctx->buf;
    if (sb->rpos + count > sb->len) return false;
    memcpy(data, &sb->data[sb->rpos], count);
    sb->rpos += count;
    return true;
}

/**
 * Returns a cmp context wired to read from the beginning of sb->data.
 * Resets sb->rpos to 0 so repeated reads start from the beginning.
 */
static inline cmp_ctx_t make_reader(ser_buf_t *sb) {
    cmp_ctx_t ctx;
    sb->rpos = 0;
    cmp_init(&ctx, sb, ser_reader, NULL, NULL);
    return ctx;
}

/**
 * Returns a cmp context wired to write into sb.
 */
static inline cmp_ctx_t make_writer(ser_buf_t *sb) {
    cmp_ctx_t ctx;
    cmp_init(&ctx, sb, NULL, NULL, ser_writer);
    return ctx;
}

/**
 * Reads and asserts the next msgpack value is a fixstr matching expected.
 */
static inline void assert_cmp_str(cmp_ctx_t *r, const char *expected) {
    char    buf[64] = {0};
    uint32_t sz     = sizeof(buf) - 1;
    assert_true(cmp_read_str(r, buf, &sz));
    assert_string_equal(buf, expected);
}

/**
 * Return pointer to the first occurrence of needle in buf, or NULL.
 */
static inline const uint8_t *find_bytes(const uint8_t *buf, size_t buf_len,
                                         const uint8_t *needle, size_t needle_len) {
    if (needle_len == 0 || needle_len > buf_len) return NULL;
    for (size_t i = 0; i <= buf_len - needle_len; ++i) {
        if (memcmp(&buf[i], needle, needle_len) == 0) return &buf[i];
    }
    return NULL;
}

/**
 * Assert that the msgpack fixstr-encoded form of string literal s appears
 * somewhere in the serialized buffer sb (.data / .len fields).
 * Strings must be ≤ 31 bytes (fixstr range); all app key/value strings qualify.
 */
#define ASSERT_SERIALIZED_STR(sb, s) do {                                   \
    static const char _s[] = (s);                                           \
    size_t _len = sizeof(_s) - 1;                                           \
    uint8_t _enc[33];                                                        \
    _enc[0] = (uint8_t)(0xa0u | _len);                                      \
    memcpy(&_enc[1], _s, _len);                                             \
    assert_non_null(find_bytes((sb).data, (sb).len, _enc, 1u + _len));      \
} while (0)
