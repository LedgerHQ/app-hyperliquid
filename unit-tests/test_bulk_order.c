#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <cmocka.h>

#include "buffer.h"
#include "hl_context.h"
#include "bulk_order.h"
#include "tlv_helpers.h"
#include "ser_helpers.h"
#include "constants.h"

/* TAG values matching bulk_order.c / order_request.c */
#define TAG_ORDER          0xdd  /* wraps an order_request inside bulk_order */
#define TAG_GROUPING       0xea
#define TAG_BUILDER        0xeb
#define TAG_BUILDER_ADDR   0xd3
#define TAG_BUILDER_FEE    0xec
/* order_request tags */
#define TAG_ORDER_TYPE     0xe0
#define TAG_ASSET          0xd1
#define TAG_IS_BUY         0xe2
#define TAG_LIMIT_PX       0xe3
#define TAG_SZ             0xe4
#define TAG_REDUCE_ONLY    0xe5
#define TAG_LIMIT_SPEC     0xd7  /* wraps limit/trigger spec inside order_request */
#define TAG_TIF            0xe6

#define TEST_ASSET_ID 2U

/* ─── context setup ──────────────────────────────────────────────────────── */

static int setup(void **state) {
    (void) state;
    ctx_reset();
    s_action_metadata m = {0};
    m.op_type  = OP_TYPE_ORDER;
    m.asset_id = TEST_ASSET_ID;
    m.network  = NETWORK_MAINNET;
    strncpy(m.asset_ticker, "ETH", sizeof(m.asset_ticker) - 1);
    ctx_save_action_metadata(&m);
    return 0;
}

static int teardown(void **state) {
    (void) state;
    ctx_reset();
    return 0;
}

/* ─── TLV builders ───────────────────────────────────────────────────────── */

static size_t build_limit_order_tlv(uint8_t *buf, uint32_t asset, bool is_buy,
                                     const char *limit_px, const char *sz) {
    uint8_t inner[16];
    size_t  inner_off = 0;
    tlv_append_uint(inner, &inner_off, TAG_TIF, 0x02 /* GTC */);

    size_t offset = 0;
    tlv_append_uint(buf, &offset, TAG_ORDER_TYPE, 0x00 /* LIMIT */);
    tlv_append_uint(buf, &offset, TAG_ASSET, asset);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, is_buy);
    tlv_append_str(buf, &offset, TAG_LIMIT_PX, limit_px);
    tlv_append_str(buf, &offset, TAG_SZ, sz);
    tlv_append_bool(buf, &offset, TAG_REDUCE_ONLY, false);
    tlv_append_field(buf, &offset, TAG_LIMIT_SPEC, inner, inner_off);
    return offset;
}

static size_t build_bulk_order(uint8_t *buf, e_grouping grouping,
                                bool with_builder, const uint8_t *builder_addr) {
    uint8_t order_tlv[256];
    size_t  order_len = build_limit_order_tlv(order_tlv, TEST_ASSET_ID, true,
                                               "3000", "0.5");
    size_t offset = 0;
    tlv_append_field(buf, &offset, TAG_ORDER, order_tlv, order_len);
    tlv_append_uint(buf, &offset, TAG_GROUPING, (uint64_t) grouping);

    if (with_builder) {
        uint8_t builder_inner[64];
        size_t  builder_off = 0;
        tlv_append_bytes(builder_inner, &builder_off, TAG_BUILDER_ADDR,
                         builder_addr, ADDRESS_LENGTH);
        tlv_append_uint(builder_inner, &builder_off, TAG_BUILDER_FEE, 100ULL);
        tlv_append_field(buf, &offset, TAG_BUILDER, builder_inner, builder_off);
    }
    return offset;
}

/* ─── parse tests ────────────────────────────────────────────────────────── */

static void test_parse_no_builder(void **state) {
    (void) state;
    uint8_t buf[512];
    size_t  len = build_bulk_order(buf, GROUPING_NA, false, NULL);

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_bulk_order(&payload, &ctx));

    assert_int_equal(result.order_count, 1);
    assert_false(result.has_builder);
    assert_int_equal(result.grouping, GROUPING_NA);
}

static void test_parse_with_builder(void **state) {
    (void) state;
    static const uint8_t builder_addr[ADDRESS_LENGTH] = {
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88, 0x99, 0x00, 0xAB, 0xCD, 0xEF, 0x01,
    };
    uint8_t buf[512];
    size_t  len = build_bulk_order(buf, GROUPING_NORMAL_TPSL, true, builder_addr);

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_bulk_order(&payload, &ctx));

    assert_true(result.has_builder);
    assert_int_equal(result.grouping, GROUPING_NORMAL_TPSL);
    assert_memory_equal(result.builder.builder, builder_addr, ADDRESS_LENGTH);
    assert_int_equal(result.builder.fee, 100ULL);
}

static void test_parse_grouping_position_tpsl(void **state) {
    (void) state;
    uint8_t buf[512];
    size_t  len = build_bulk_order(buf, GROUPING_POSITION_TPSL, false, NULL);

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_bulk_order(&payload, &ctx));
    assert_int_equal(result.grouping, GROUPING_POSITION_TPSL);
}

static void test_parse_max_orders(void **state) {
    (void) state;
    uint8_t buf[2048];
    size_t  offset = 0;

    for (int i = 0; i < BULK_MAX_SIZE; ++i) {
        uint8_t order_tlv[256];
        size_t  order_len = build_limit_order_tlv(order_tlv, TEST_ASSET_ID, true, "100", "1");
        tlv_append_field(buf, &offset, TAG_ORDER, order_tlv, order_len);
    }
    tlv_append_uint(buf, &offset, TAG_GROUPING, GROUPING_NA);

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_true(parse_bulk_order(&payload, &ctx));
    assert_int_equal(result.order_count, BULK_MAX_SIZE);
}

static void test_parse_too_many_orders_fails(void **state) {
    (void) state;
    uint8_t buf[4096];
    size_t  offset = 0;

    for (int i = 0; i < BULK_MAX_SIZE + 1; ++i) {
        uint8_t order_tlv[256];
        size_t  order_len = build_limit_order_tlv(order_tlv, TEST_ASSET_ID, true, "100", "1");
        tlv_append_field(buf, &offset, TAG_ORDER, order_tlv, order_len);
    }
    tlv_append_uint(buf, &offset, TAG_GROUPING, GROUPING_NA);

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_order(&payload, &ctx));
}

static void test_parse_missing_grouping_fails(void **state) {
    (void) state;
    uint8_t order_tlv[256];
    size_t  order_len = build_limit_order_tlv(order_tlv, TEST_ASSET_ID, true, "100", "1");

    uint8_t buf[512];
    size_t  offset = 0;
    tlv_append_field(buf, &offset, TAG_ORDER, order_tlv, order_len);
    /* No TAG_GROUPING */

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_order(&payload, &ctx));
}

static void test_parse_invalid_grouping_fails(void **state) {
    (void) state;
    uint8_t order_tlv[256];
    size_t  order_len = build_limit_order_tlv(order_tlv, TEST_ASSET_ID, true, "100", "1");

    uint8_t buf[512];
    size_t  offset = 0;
    tlv_append_field(buf, &offset, TAG_ORDER, order_tlv, order_len);
    tlv_append_uint(buf, &offset, TAG_GROUPING, 0xFF);

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_order(&payload, &ctx));
}

static void test_parse_missing_order_fails(void **state) {
    (void) state;
    /* No orders, only grouping — TAG_ORDER is required */
    uint8_t buf[16];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_GROUPING, GROUPING_NA);

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_order(&payload, &ctx));
}

static void test_parse_builder_missing_address_fails(void **state) {
    (void) state;
    /* builder_info with only fee, no address */
    uint8_t order_tlv[256];
    size_t  order_len = build_limit_order_tlv(order_tlv, TEST_ASSET_ID, true, "100", "1");

    uint8_t builder_inner[32];
    size_t  builder_off = 0;
    tlv_append_uint(builder_inner, &builder_off, TAG_BUILDER_FEE, 50ULL);

    uint8_t buf[512];
    size_t  offset = 0;
    tlv_append_field(buf, &offset, TAG_ORDER, order_tlv, order_len);
    tlv_append_uint(buf, &offset, TAG_GROUPING, GROUPING_NA);
    tlv_append_field(buf, &offset, TAG_BUILDER, builder_inner, builder_off);

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_order(&payload, &ctx));
}

static void test_parse_builder_wrong_address_size_fails(void **state) {
    (void) state;
    uint8_t order_tlv[256];
    size_t  order_len = build_limit_order_tlv(order_tlv, TEST_ASSET_ID, true, "100", "1");

    /* 10 bytes instead of ADDRESS_LENGTH (20) */
    static const uint8_t short_addr[10] = {0};
    uint8_t builder_inner[64];
    size_t  builder_off = 0;
    tlv_append_bytes(builder_inner, &builder_off, TAG_BUILDER_ADDR, short_addr, sizeof(short_addr));
    tlv_append_uint(builder_inner, &builder_off, TAG_BUILDER_FEE, 50ULL);

    uint8_t buf[512];
    size_t  offset = 0;
    tlv_append_field(buf, &offset, TAG_ORDER, order_tlv, order_len);
    tlv_append_uint(buf, &offset, TAG_GROUPING, GROUPING_NA);
    tlv_append_field(buf, &offset, TAG_BUILDER, builder_inner, builder_off);

    s_bulk_order     result = {0};
    s_bulk_order_ctx ctx    = {.bulk_order = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_order(&payload, &ctx));
}

/* ─── serialization tests ────────────────────────────────────────────────── */

static void test_serialize_no_builder(void **state) {
    (void) state;
    s_bulk_order bo = {0};
    bo.order_count = 1;
    bo.grouping    = GROUPING_NA;
    bo.has_builder = false;
    bo.orders[0].asset      = TEST_ASSET_ID;
    bo.orders[0].is_buy     = true;
    bo.orders[0].order_type = ORDER_TYPE_LIMIT;
    strncpy(bo.orders[0].limit_px, "3000", sizeof(bo.orders[0].limit_px) - 1);
    strncpy(bo.orders[0].sz, "0.5", sizeof(bo.orders[0].sz) - 1);
    bo.orders[0].limit.tif = TIF_GTC;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(bulk_order_serialize(&bo, &cmp));

    /* Top-level map must have 3 entries (no builder): fixmap(3) = 0x83 */
    assert_true(sb.len > 0);
    assert_int_equal(sb.data[0], 0x83);

    /* Required keys/values must appear in the serialized bytes */
    ASSERT_SERIALIZED_STR(sb, "type");
    ASSERT_SERIALIZED_STR(sb, "order");
    ASSERT_SERIALIZED_STR(sb, "orders");
    ASSERT_SERIALIZED_STR(sb, "grouping");
    ASSERT_SERIALIZED_STR(sb, "na");
}

static void test_serialize_with_builder_hex_encoding(void **state) {
    (void) state;
    /* Verify that builder address is hex-encoded as "0x<40 lowercase hex chars>" */
    static const uint8_t builder_addr[ADDRESS_LENGTH] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0x00, 0x11, 0x22, 0x33,
        0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD,
    };

    s_bulk_order bo = {0};
    bo.order_count = 1;
    bo.grouping    = GROUPING_NA;
    bo.has_builder = true;
    memcpy(bo.builder.builder, builder_addr, ADDRESS_LENGTH);
    bo.builder.fee = 50ULL;
    bo.orders[0].asset      = TEST_ASSET_ID;
    bo.orders[0].is_buy     = true;
    bo.orders[0].order_type = ORDER_TYPE_LIMIT;
    strncpy(bo.orders[0].limit_px, "1", sizeof(bo.orders[0].limit_px) - 1);
    strncpy(bo.orders[0].sz, "1", sizeof(bo.orders[0].sz) - 1);
    bo.orders[0].limit.tif = TIF_GTC;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(bulk_order_serialize(&bo, &cmp));

    /* Top-level map must have 4 entries: fixmap(4) = 0x84 */
    assert_true(sb.len > 0);
    assert_int_equal(sb.data[0], 0x84);

    ASSERT_SERIALIZED_STR(sb, "builder");
    ASSERT_SERIALIZED_STR(sb, "b");

    /* "0xdeadbeef..." is 42 chars — msgpack str8: 0xd9 0x2a + bytes */
    static const char expected_addr[] = "0xdeadbeefcafe00112233445566778899aabbccdd";
    uint8_t enc[44];
    enc[0] = 0xd9; enc[1] = 42;
    memcpy(&enc[2], expected_addr, 42);
    assert_non_null(find_bytes(sb.data, sb.len, enc, sizeof(enc)));
}

static void test_serialize_grouping_strings(void **state) {
    (void) state;
    /* Verify each grouping enum maps to the correct msgpack string */
    static const struct { e_grouping g; const char *expected; } cases[] = {
        { GROUPING_NA,            "na"           },
        { GROUPING_NORMAL_TPSL,   "normalTpsl"   },
        { GROUPING_POSITION_TPSL, "positionTpsl" },
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        s_bulk_order bo = {0};
        bo.order_count = 1;
        bo.grouping    = cases[i].g;
        bo.orders[0].asset      = TEST_ASSET_ID;
        bo.orders[0].is_buy     = true;
        bo.orders[0].order_type = ORDER_TYPE_LIMIT;
        strncpy(bo.orders[0].limit_px, "1", sizeof(bo.orders[0].limit_px) - 1);
        strncpy(bo.orders[0].sz, "1", sizeof(bo.orders[0].sz) - 1);
        bo.orders[0].limit.tif = TIF_GTC;

        ser_buf_t sb  = {0};
        cmp_ctx_t cmp = make_writer(&sb);
        assert_true(bulk_order_serialize(&bo, &cmp));

        /* Verify the grouping key is present */
        ASSERT_SERIALIZED_STR(sb, "grouping");

        /* Verify the correct grouping value string is present */
        size_t vlen = strlen(cases[i].expected);
        uint8_t enc[14];
        enc[0] = (uint8_t)(0xa0 | vlen);
        memcpy(&enc[1], cases[i].expected, vlen);
        assert_non_null(find_bytes(sb.data, sb.len, enc, 1 + vlen));
    }
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_parse_no_builder, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_with_builder, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_grouping_position_tpsl, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_max_orders, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_too_many_orders_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_grouping_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_invalid_grouping_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_order_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_builder_missing_address_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_builder_wrong_address_size_fails, setup, teardown),
        cmocka_unit_test(test_serialize_no_builder),
        cmocka_unit_test(test_serialize_with_builder_hex_encoding),
        cmocka_unit_test(test_serialize_grouping_strings),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
