#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "buffer.h"
#include "hl_context.h"
#include "order_request.h"
#include "tlv_helpers.h"
#include "ser_helpers.h"
#include "constants.h"

/* TAG values matching the parser definitions in order_request.c */
#define TAG_ORDER_TYPE  0xe0
#define TAG_ASSET       0xd1
#define TAG_IS_BUY      0xe2
#define TAG_LIMIT_PX    0xe3
#define TAG_SZ          0xe4
#define TAG_REDUCE_ONLY 0xe5
#define TAG_ORDER       0xd7
#define TAG_CLOID       0xee
/* Nested limit order tags */
#define TAG_TIF         0xe6
/* Nested trigger order tags */
#define TAG_IS_MARKET   0xe7
#define TAG_TRIGGER_PX  0xe8
#define TAG_TRIGGER_TYPE 0xe9

#define TEST_ASSET_ID 3U

/* ─── context setup ──────────────────────────────────────────────────────── */

static int setup(void **state) {
    (void) state;
    ctx_reset();
    s_action_metadata m = {0};
    m.op_type  = OP_TYPE_ORDER;
    m.asset_id = TEST_ASSET_ID;
    m.network  = NETWORK_MAINNET;
    strncpy(m.asset_ticker, "BTC", sizeof(m.asset_ticker) - 1);
    ctx_save_action_metadata(&m);
    return 0;
}

static int teardown(void **state) {
    (void) state;
    ctx_reset();
    return 0;
}

/* ─── TLV builders ───────────────────────────────────────────────────────── */

/**
 * Appends a nested limit order TLV (tag 0xd7 wrapping a tif tag) to buf.
 */
static void append_limit_order_payload(uint8_t *outer, size_t *outer_off, e_tif tif) {
    uint8_t inner[16];
    size_t  inner_off = 0;
    tlv_append_uint(inner, &inner_off, TAG_TIF, (uint64_t) tif);
    tlv_append_field(outer, outer_off, TAG_ORDER, inner, inner_off);
}

/**
 * Appends a nested trigger order TLV to buf.
 */
static void append_trigger_order_payload(uint8_t        *outer,
                                          size_t         *outer_off,
                                          bool            is_market,
                                          const char     *trigger_px,
                                          e_trigger_type  tpsl) {
    uint8_t inner[64];
    size_t  inner_off = 0;
    tlv_append_bool(inner, &inner_off, TAG_IS_MARKET, is_market);
    tlv_append_str(inner, &inner_off, TAG_TRIGGER_PX, trigger_px);
    tlv_append_uint(inner, &inner_off, TAG_TRIGGER_TYPE, (uint64_t) tpsl);
    tlv_append_field(outer, outer_off, TAG_ORDER, inner, inner_off);
}

/**
 * Builds a complete limit order_request TLV payload.
 */
static size_t build_limit_order_request(uint8_t    *buf,
                                         uint32_t    asset,
                                         bool        is_buy,
                                         const char *limit_px,
                                         const char *sz,
                                         bool        reduce_only,
                                         e_tif       tif) {
    size_t offset = 0;
    tlv_append_uint(buf, &offset, TAG_ORDER_TYPE, ORDER_TYPE_LIMIT);
    tlv_append_uint(buf, &offset, TAG_ASSET, asset);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, is_buy);
    tlv_append_str(buf, &offset, TAG_LIMIT_PX, limit_px);
    tlv_append_str(buf, &offset, TAG_SZ, sz);
    tlv_append_bool(buf, &offset, TAG_REDUCE_ONLY, reduce_only);
    append_limit_order_payload(buf, &offset, tif);
    return offset;
}

/**
 * Builds a complete trigger order_request TLV payload.
 */
static size_t build_trigger_order_request(uint8_t        *buf,
                                           uint32_t        asset,
                                           bool            is_buy,
                                           const char     *limit_px,
                                           const char     *sz,
                                           bool            reduce_only,
                                           bool            is_market,
                                           const char     *trigger_px,
                                           e_trigger_type  tpsl) {
    size_t offset = 0;
    tlv_append_uint(buf, &offset, TAG_ORDER_TYPE, ORDER_TYPE_TRIGGER);
    tlv_append_uint(buf, &offset, TAG_ASSET, asset);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, is_buy);
    tlv_append_str(buf, &offset, TAG_LIMIT_PX, limit_px);
    tlv_append_str(buf, &offset, TAG_SZ, sz);
    tlv_append_bool(buf, &offset, TAG_REDUCE_ONLY, reduce_only);
    append_trigger_order_payload(buf, &offset, is_market, trigger_px, tpsl);
    return offset;
}

/* ─── parse tests: limit orders ──────────────────────────────────────────── */

static void test_parse_limit_order_gtc(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  len = build_limit_order_request(buf, TEST_ASSET_ID, true,
                                             "50000.5", "0.1", false, TIF_GTC);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_order_request(&payload, &ctx));

    assert_int_equal(result.order_type, ORDER_TYPE_LIMIT);
    assert_int_equal(result.asset, TEST_ASSET_ID);
    assert_true(result.is_buy);
    assert_string_equal(result.limit_px, "50000.5");
    assert_string_equal(result.sz, "0.1");
    assert_false(result.reduce_only);
    assert_int_equal(result.limit.tif, TIF_GTC);
}

static void test_parse_limit_order_ioc(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  len = build_limit_order_request(buf, TEST_ASSET_ID, false,
                                             "100", "2.5", true, TIF_IOC);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_order_request(&payload, &ctx));

    assert_int_equal(result.order_type, ORDER_TYPE_LIMIT);
    assert_false(result.is_buy);
    assert_true(result.reduce_only);
    assert_int_equal(result.limit.tif, TIF_IOC);
}

static void test_parse_limit_order_alo(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  len = build_limit_order_request(buf, TEST_ASSET_ID, true,
                                             "1", "1", false, TIF_ALO);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_order_request(&payload, &ctx));
    assert_int_equal(result.limit.tif, TIF_ALO);
}

/* ─── parse tests: trigger orders ───────────────────────────────────────── */

static void test_parse_trigger_order_tp(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  len = build_trigger_order_request(buf, TEST_ASSET_ID, true,
                                               "55000", "0.5", false,
                                               true, "56000", TRIGGER_TYPE_TP);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_order_request(&payload, &ctx));

    assert_int_equal(result.order_type, ORDER_TYPE_TRIGGER);
    assert_true(result.trigger.is_market);
    assert_string_equal(result.trigger.trigger_px, "56000");
    assert_int_equal(result.trigger.tpsl, TRIGGER_TYPE_TP);
}

static void test_parse_trigger_order_sl(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  len = build_trigger_order_request(buf, TEST_ASSET_ID, false,
                                               "48000", "1", true,
                                               false, "47000", TRIGGER_TYPE_SL);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_order_request(&payload, &ctx));

    assert_int_equal(result.trigger.tpsl, TRIGGER_TYPE_SL);
    assert_false(result.trigger.is_market);
    assert_string_equal(result.trigger.trigger_px, "47000");
}

/* ─── parse tests: error paths ───────────────────────────────────────────── */

static void test_parse_asset_mismatch_fails(void **state) {
    (void) state;
    uint8_t buf[256];
    /* Asset 99 != TEST_ASSET_ID */
    size_t len = build_limit_order_request(buf, 99, true, "100", "1", false, TIF_GTC);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_order_request(&payload, &ctx));
}

static void test_parse_missing_order_type_fails(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  offset = 0;
    /* Omit TAG_ORDER_TYPE */
    tlv_append_uint(buf, &offset, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, true);
    tlv_append_str(buf, &offset, TAG_LIMIT_PX, "100");
    tlv_append_str(buf, &offset, TAG_SZ, "1");
    tlv_append_bool(buf, &offset, TAG_REDUCE_ONLY, false);
    /* Append a limit order (TAG_ORDER requires TAG_ORDER_TYPE first) */
    uint8_t inner[8];
    size_t  inner_off = 0;
    tlv_append_uint(inner, &inner_off, TAG_TIF, TIF_GTC);
    tlv_append_field(buf, &offset, TAG_ORDER, inner, inner_off);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_order_request(&payload, &ctx));
}

static void test_parse_missing_limit_px_fails(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_ORDER_TYPE, ORDER_TYPE_LIMIT);
    tlv_append_uint(buf, &offset, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, true);
    /* Omit TAG_LIMIT_PX */
    tlv_append_str(buf, &offset, TAG_SZ, "1");
    tlv_append_bool(buf, &offset, TAG_REDUCE_ONLY, false);
    uint8_t inner[8];
    size_t  inner_off = 0;
    tlv_append_uint(inner, &inner_off, TAG_TIF, TIF_GTC);
    tlv_append_field(buf, &offset, TAG_ORDER, inner, inner_off);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_order_request(&payload, &ctx));
}

/* ─── string boundary tests ──────────────────────────────────────────────── */

static void test_parse_max_length_limit_px_succeeds(void **state) {
    (void) state;
    /* NUMERIC_STRING_LENGTH = 21; buffer is [22]; a 21-char string exactly fits */
    char max_str[NUMERIC_STRING_LENGTH + 1];
    memset(max_str, '9', NUMERIC_STRING_LENGTH);
    max_str[NUMERIC_STRING_LENGTH] = '\0';

    uint8_t buf[512];
    size_t  len = build_limit_order_request(buf, TEST_ASSET_ID, true,
                                             max_str, "1", false, TIF_GTC);
    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_order_request(&payload, &ctx));
    assert_string_equal(result.limit_px, max_str);
}

static void test_parse_too_long_limit_px_fails(void **state) {
    (void) state;
    /* 22 chars + NUL = 23 bytes needed, buffer is only 22 → must fail */
    char over_str[NUMERIC_STRING_LENGTH + 2 + 1];
    memset(over_str, '9', NUMERIC_STRING_LENGTH + 2);
    over_str[NUMERIC_STRING_LENGTH + 2] = '\0';

    uint8_t buf[512];
    size_t  len = build_limit_order_request(buf, TEST_ASSET_ID, true,
                                             over_str, "1", false, TIF_GTC);
    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_order_request(&payload, &ctx));
}

static void test_parse_empty_limit_px_fails(void **state) {
    (void) state;
    /* min_length=1 enforced in handle_limit_px */
    uint8_t buf[256];
    size_t  len = build_limit_order_request(buf, TEST_ASSET_ID, true,
                                             "", "1", false, TIF_GTC);
    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_order_request(&payload, &ctx));
}

/* ─── invalid enum value tests ───────────────────────────────────────────── */

static void test_parse_invalid_order_type_fails(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_ORDER_TYPE, 0xFF);
    tlv_append_uint(buf, &offset, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, true);
    tlv_append_str(buf, &offset, TAG_LIMIT_PX, "100");
    tlv_append_str(buf, &offset, TAG_SZ, "1");
    tlv_append_bool(buf, &offset, TAG_REDUCE_ONLY, false);
    uint8_t inner[8];
    size_t  inner_off = 0;
    tlv_append_uint(inner, &inner_off, TAG_TIF, TIF_GTC);
    tlv_append_field(buf, &offset, TAG_ORDER, inner, inner_off);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_order_request(&payload, &ctx));
}

static void test_parse_invalid_tif_fails(void **state) {
    (void) state;
    uint8_t buf[256];
    /* Build manually with tif=0xFF */
    size_t offset = 0;
    tlv_append_uint(buf, &offset, TAG_ORDER_TYPE, ORDER_TYPE_LIMIT);
    tlv_append_uint(buf, &offset, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, true);
    tlv_append_str(buf, &offset, TAG_LIMIT_PX, "100");
    tlv_append_str(buf, &offset, TAG_SZ, "1");
    tlv_append_bool(buf, &offset, TAG_REDUCE_ONLY, false);
    uint8_t inner[8];
    size_t  inner_off = 0;
    tlv_append_uint(inner, &inner_off, TAG_TIF, 0xFF);
    tlv_append_field(buf, &offset, TAG_ORDER, inner, inner_off);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_order_request(&payload, &ctx));
}

static void test_parse_invalid_trigger_type_fails(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_ORDER_TYPE, ORDER_TYPE_TRIGGER);
    tlv_append_uint(buf, &offset, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, true);
    tlv_append_str(buf, &offset, TAG_LIMIT_PX, "100");
    tlv_append_str(buf, &offset, TAG_SZ, "1");
    tlv_append_bool(buf, &offset, TAG_REDUCE_ONLY, false);
    uint8_t inner[64];
    size_t  inner_off = 0;
    tlv_append_bool(inner, &inner_off, TAG_IS_MARKET, true);
    tlv_append_str(inner, &inner_off, TAG_TRIGGER_PX, "100");
    tlv_append_uint(inner, &inner_off, TAG_TRIGGER_TYPE, 0xFF);
    tlv_append_field(buf, &offset, TAG_ORDER, inner, inner_off);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_order_request(&payload, &ctx));
}

/* ─── malformed / truncated TLV tests ───────────────────────────────────── */

static void test_parse_truncated_payload_fails(void **state) {
    (void) state;
    /* Single tag byte — no length or value follows */
    uint8_t  truncated[] = {TAG_ORDER_TYPE};
    buffer_t payload     = make_buffer(truncated, sizeof(truncated));

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    assert_false(parse_order_request(&payload, &ctx));
}

static void test_parse_oversized_length_field_fails(void **state) {
    (void) state;
    /* tag=TAG_ORDER_TYPE, length=50 (claims 50 bytes), only 3 bytes follow */
    uint8_t  oversized[] = {TAG_ORDER_TYPE, 0x32, 0x01, 0x02, 0x03};
    buffer_t payload     = make_buffer(oversized, sizeof(oversized));

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    assert_false(parse_order_request(&payload, &ctx));
}

/* ─── zero value tests ───────────────────────────────────────────────────── */

static void test_parse_zero_asset_mismatch_fails(void **state) {
    (void) state;
    /* asset=0 does not match TEST_ASSET_ID=3 → should fail */
    uint8_t buf[256];
    size_t  len = build_limit_order_request(buf, 0, true, "100", "1", false, TIF_GTC);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_order_request(&payload, &ctx));
}

/* ─── serialization tests ────────────────────────────────────────────────── */

static void test_serialize_limit_order(void **state) {
    (void) state;
    s_order_request req = {
        .asset       = TEST_ASSET_ID,
        .is_buy      = true,
        .reduce_only = false,
        .order_type  = ORDER_TYPE_LIMIT,
    };
    strncpy(req.limit_px, "50000", sizeof(req.limit_px) - 1);
    strncpy(req.sz, "0.1", sizeof(req.sz) - 1);
    req.limit.tif = TIF_GTC;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(order_request_serialize(&req, &cmp));

    /* fixmap(6) = 0x86 */
    assert_int_equal(sb.data[0], 0x86);
    /* Verify the nested limit/tif structure key names and GTC value */
    ASSERT_SERIALIZED_STR(sb, "t");
    ASSERT_SERIALIZED_STR(sb, "limit");
    ASSERT_SERIALIZED_STR(sb, "tif");
    ASSERT_SERIALIZED_STR(sb, "Gtc");
}

static void test_serialize_limit_alo(void **state) {
    (void) state;
    s_order_request req = {
        .asset       = TEST_ASSET_ID,
        .is_buy      = true,
        .reduce_only = false,
        .order_type  = ORDER_TYPE_LIMIT,
    };
    strncpy(req.limit_px, "50000", sizeof(req.limit_px) - 1);
    strncpy(req.sz, "0.1", sizeof(req.sz) - 1);
    req.limit.tif = TIF_ALO;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(order_request_serialize(&req, &cmp));

    assert_int_equal(sb.data[0], 0x86);
    ASSERT_SERIALIZED_STR(sb, "Alo");
}

static void test_serialize_limit_ioc(void **state) {
    (void) state;
    s_order_request req = {
        .asset       = TEST_ASSET_ID,
        .is_buy      = true,
        .reduce_only = false,
        .order_type  = ORDER_TYPE_LIMIT,
    };
    strncpy(req.limit_px, "50000", sizeof(req.limit_px) - 1);
    strncpy(req.sz, "0.1", sizeof(req.sz) - 1);
    req.limit.tif = TIF_IOC;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(order_request_serialize(&req, &cmp));

    assert_int_equal(sb.data[0], 0x86);
    ASSERT_SERIALIZED_STR(sb, "Ioc");
}

static void test_serialize_trigger_order(void **state) {
    (void) state;
    s_order_request req = {
        .asset       = TEST_ASSET_ID,
        .is_buy      = false,
        .reduce_only = true,
        .order_type  = ORDER_TYPE_TRIGGER,
    };
    strncpy(req.limit_px, "48000", sizeof(req.limit_px) - 1);
    strncpy(req.sz, "2", sizeof(req.sz) - 1);
    req.trigger.is_market = false;
    strncpy(req.trigger.trigger_px, "47000", sizeof(req.trigger.trigger_px) - 1);
    req.trigger.tpsl = TRIGGER_TYPE_SL;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(order_request_serialize(&req, &cmp));

    /* fixmap(6) = 0x86 */
    assert_int_equal(sb.data[0], 0x86);
    /* Verify the nested trigger structure key names and SL value */
    ASSERT_SERIALIZED_STR(sb, "t");
    ASSERT_SERIALIZED_STR(sb, "trigger");
    ASSERT_SERIALIZED_STR(sb, "tpsl");
    ASSERT_SERIALIZED_STR(sb, "sl");
    ASSERT_SERIALIZED_STR(sb, "triggerPx");
    ASSERT_SERIALIZED_STR(sb, "isMarket");
}

static void test_serialize_trigger_tp(void **state) {
    (void) state;
    s_order_request req = {
        .asset       = TEST_ASSET_ID,
        .is_buy      = true,
        .reduce_only = false,
        .order_type  = ORDER_TYPE_TRIGGER,
    };
    strncpy(req.limit_px, "51000", sizeof(req.limit_px) - 1);
    strncpy(req.sz, "1", sizeof(req.sz) - 1);
    req.trigger.is_market = true;
    strncpy(req.trigger.trigger_px, "51000", sizeof(req.trigger.trigger_px) - 1);
    req.trigger.tpsl = TRIGGER_TYPE_TP;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(order_request_serialize(&req, &cmp));

    assert_int_equal(sb.data[0], 0x86);
    ASSERT_SERIALIZED_STR(sb, "trigger");
    ASSERT_SERIALIZED_STR(sb, "tpsl");
    ASSERT_SERIALIZED_STR(sb, "tp");
}

/* ─── CLOID tests ────────────────────────────────────────────────────────── */

/* Reference 16-byte CLOID and its expected hex string representation. */
static const uint8_t TEST_CLOID_BYTES[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};
#define TEST_CLOID_HEX "0x0123456789abcdeffedcba9876543210"

static void test_parse_cloid_valid(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  len = build_limit_order_request(buf, TEST_ASSET_ID, true,
                                             "50000", "1", false, TIF_GTC);
    tlv_append_bytes(buf, &len, TAG_CLOID, TEST_CLOID_BYTES, sizeof(TEST_CLOID_BYTES));

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_order_request(&payload, &ctx));

    assert_true(result.has_cloid);
    assert_string_equal(result.cloid, TEST_CLOID_HEX);
}

static void test_parse_cloid_too_short_fails(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  len = build_limit_order_request(buf, TEST_ASSET_ID, true,
                                             "50000", "1", false, TIF_GTC);
    tlv_append_bytes(buf, &len, TAG_CLOID, TEST_CLOID_BYTES, 15);

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_order_request(&payload, &ctx));
}

static void test_parse_cloid_too_long_fails(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  len = build_limit_order_request(buf, TEST_ASSET_ID, true,
                                             "50000", "1", false, TIF_GTC);
    uint8_t oversized[17] = {0};
    memcpy(oversized, TEST_CLOID_BYTES, sizeof(TEST_CLOID_BYTES));
    tlv_append_bytes(buf, &len, TAG_CLOID, oversized, sizeof(oversized));

    s_order_request     result = {0};
    s_order_request_ctx ctx    = {.order_request = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_order_request(&payload, &ctx));
}

static void test_serialize_limit_order_with_cloid(void **state) {
    (void) state;
    s_order_request req = {
        .asset       = TEST_ASSET_ID,
        .is_buy      = true,
        .reduce_only = false,
        .order_type  = ORDER_TYPE_LIMIT,
        .has_cloid   = true,
    };
    strncpy(req.limit_px, "50000", sizeof(req.limit_px) - 1);
    strncpy(req.sz, "0.1", sizeof(req.sz) - 1);
    req.limit.tif = TIF_GTC;
    strcpy(req.cloid, TEST_CLOID_HEX);

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(order_request_serialize(&req, &cmp));

    /* fixmap(7) = 0x87: 6 base fields + "c" cloid field */
    assert_int_equal(sb.data[0], 0x87);

    /* "c" key present (fixstr len 1) */
    ASSERT_SERIALIZED_STR(sb, "c");

    /* Cloid value: str8 marker (0xd9), length 34 (0x22), then the hex string */
    uint8_t expected_val[36];
    expected_val[0] = 0xd9;
    expected_val[1] = 0x22;
    memcpy(&expected_val[2], TEST_CLOID_HEX, 34);
    assert_non_null(find_bytes(sb.data, sb.len, expected_val, sizeof(expected_val)));
}

static void test_serialize_limit_order_no_cloid(void **state) {
    (void) state;
    s_order_request req = {
        .asset       = TEST_ASSET_ID,
        .is_buy      = true,
        .reduce_only = false,
        .order_type  = ORDER_TYPE_LIMIT,
        .has_cloid   = false,
    };
    strncpy(req.limit_px, "50000", sizeof(req.limit_px) - 1);
    strncpy(req.sz, "0.1", sizeof(req.sz) - 1);
    req.limit.tif = TIF_GTC;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(order_request_serialize(&req, &cmp));

    /* fixmap(6) = 0x86 — no cloid field added */
    assert_int_equal(sb.data[0], 0x86);

    /* "c" key must not appear */
    static const uint8_t c_key[] = {0xa1, 'c'};
    assert_null(find_bytes(sb.data, sb.len, c_key, sizeof(c_key)));
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_parse_limit_order_gtc, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_limit_order_ioc, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_limit_order_alo, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_trigger_order_tp, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_trigger_order_sl, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_asset_mismatch_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_order_type_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_limit_px_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_max_length_limit_px_succeeds, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_too_long_limit_px_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_empty_limit_px_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_invalid_order_type_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_invalid_tif_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_invalid_trigger_type_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_truncated_payload_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_oversized_length_field_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_zero_asset_mismatch_fails, setup, teardown),
        cmocka_unit_test(test_serialize_limit_order),
        cmocka_unit_test(test_serialize_limit_alo),
        cmocka_unit_test(test_serialize_limit_ioc),
        cmocka_unit_test(test_serialize_trigger_order),
        cmocka_unit_test(test_serialize_trigger_tp),
        cmocka_unit_test_setup_teardown(test_parse_cloid_valid, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_cloid_too_short_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_cloid_too_long_fails, setup, teardown),
        cmocka_unit_test(test_serialize_limit_order_with_cloid),
        cmocka_unit_test(test_serialize_limit_order_no_cloid),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
