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
#include "bulk_modify.h"
#include "tlv_helpers.h"
#include "ser_helpers.h"

/* TAG values matching bulk_modify.c / order_request.c */
#define TAG_MODIFY_REQUEST  0xd8
#define TAG_MODIFY_ORDER    0xdd  /* wraps an order_request inside modify_request */
#define TAG_OID             0xdc
/* order_request tags */
#define TAG_ORDER_TYPE      0xe0
#define TAG_ASSET           0xd1
#define TAG_IS_BUY          0xe2
#define TAG_LIMIT_PX        0xe3
#define TAG_SZ              0xe4
#define TAG_REDUCE_ONLY     0xe5
#define TAG_LIMIT_SPEC      0xd7  /* wraps limit/trigger spec inside order_request */
#define TAG_TIF             0xe6

#define TEST_ASSET_ID 5U

/* ─── context setup ──────────────────────────────────────────────────────── */

static int setup(void **state) {
    (void) state;
    ctx_reset();
    s_action_metadata m = {0};
    m.op_type  = OP_TYPE_MODIFY;
    m.asset_id = TEST_ASSET_ID;
    m.network  = NETWORK_MAINNET;
    strncpy(m.asset_ticker, "SOL", sizeof(m.asset_ticker) - 1);
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
 * Builds a limit order_request TLV nested inside a modify_request.
 */
static size_t build_modify_request(uint8_t    *buf,
                                   uint64_t    oid,
                                   uint32_t    asset,
                                   bool        is_buy,
                                   const char *limit_px,
                                   const char *sz) {
    /* build inner limit order TLV */
    uint8_t order_inner[16];
    size_t  order_inner_off = 0;
    tlv_append_uint(order_inner, &order_inner_off, TAG_TIF, 0x02 /* GTC */);

    uint8_t order_tlv[256];
    size_t  order_off = 0;
    tlv_append_uint(order_tlv, &order_off, TAG_ORDER_TYPE, 0x00 /* LIMIT */);
    tlv_append_uint(order_tlv, &order_off, TAG_ASSET, asset);
    tlv_append_bool(order_tlv, &order_off, TAG_IS_BUY, is_buy);
    tlv_append_str(order_tlv, &order_off, TAG_LIMIT_PX, limit_px);
    tlv_append_str(order_tlv, &order_off, TAG_SZ, sz);
    tlv_append_bool(order_tlv, &order_off, TAG_REDUCE_ONLY, false);
    tlv_append_field(order_tlv, &order_off, TAG_LIMIT_SPEC, order_inner, order_inner_off);

    /* build modify_request: TAG_MODIFY_ORDER + TAG_OID */
    uint8_t modify_inner[512];
    size_t  modify_off = 0;
    tlv_append_field(modify_inner, &modify_off, TAG_MODIFY_ORDER, order_tlv, order_off);
    tlv_append_uint(modify_inner, &modify_off, TAG_OID, oid);

    /* wrap in TAG_MODIFY_REQUEST */
    size_t offset = 0;
    tlv_append_field(buf, &offset, TAG_MODIFY_REQUEST, modify_inner, modify_off);
    return offset;
}

/* ─── parse tests ────────────────────────────────────────────────────────── */

static void test_parse_single_modify(void **state) {
    (void) state;
    uint8_t buf[512];
    size_t  len = build_modify_request(buf, 999ULL, TEST_ASSET_ID, true, "50000", "0.5");

    s_bulk_modify     result = {0};
    s_bulk_modify_ctx ctx    = {.bulk_modify = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_bulk_modify(&payload, &ctx));

    assert_int_equal(result.modify_count, 1);
    assert_int_equal(result.modifies[0].oid, 999ULL);
    assert_string_equal(result.modifies[0].order.limit_px, "50000");
    assert_string_equal(result.modifies[0].order.sz, "0.5");
    assert_true(result.modifies[0].order.is_buy);
}

static void test_parse_max_modifies(void **state) {
    (void) state;
    uint8_t buf[2048];
    size_t  offset = 0;

    for (uint64_t i = 0; i < BULK_MAX_SIZE; ++i) {
        uint8_t req[512];
        char    px[16];
        snprintf(px, sizeof(px), "%llu", (unsigned long long)(i + 1));
        size_t req_len = build_modify_request(req, i + 1, TEST_ASSET_ID, true, px, "1");
        /* buf already has the outer TAG_MODIFY_REQUEST wrapper; append raw bytes */
        memcpy(&buf[offset], req, req_len);
        offset += req_len;
    }

    s_bulk_modify     result = {0};
    s_bulk_modify_ctx ctx    = {.bulk_modify = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_true(parse_bulk_modify(&payload, &ctx));
    assert_int_equal(result.modify_count, BULK_MAX_SIZE);
}

static void test_parse_too_many_modifies_fails(void **state) {
    (void) state;
    uint8_t buf[4096];
    size_t  offset = 0;

    for (uint64_t i = 0; i < (uint64_t)(BULK_MAX_SIZE + 1); ++i) {
        uint8_t req[512];
        size_t  req_len = build_modify_request(req, i + 1, TEST_ASSET_ID, true, "100", "1");
        memcpy(&buf[offset], req, req_len);
        offset += req_len;
    }

    s_bulk_modify     result = {0};
    s_bulk_modify_ctx ctx    = {.bulk_modify = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_modify(&payload, &ctx));
}

static void test_parse_missing_modify_request_fails(void **state) {
    (void) state;
    uint8_t  empty[1] = {0};
    buffer_t payload  = make_buffer(empty, 0);

    s_bulk_modify     result = {0};
    s_bulk_modify_ctx ctx    = {.bulk_modify = &result};

    assert_false(parse_bulk_modify(&payload, &ctx));
}

static void test_parse_missing_oid_in_modify_request_fails(void **state) {
    (void) state;
    /* Build a modify_request without TAG_OID */
    uint8_t order_inner[16];
    size_t  order_inner_off = 0;
    tlv_append_uint(order_inner, &order_inner_off, TAG_TIF, 0x02);

    uint8_t order_tlv[256];
    size_t  order_off = 0;
    tlv_append_uint(order_tlv, &order_off, TAG_ORDER_TYPE, 0x00);
    tlv_append_uint(order_tlv, &order_off, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_bool(order_tlv, &order_off, TAG_IS_BUY, true);
    tlv_append_str(order_tlv, &order_off, TAG_LIMIT_PX, "100");
    tlv_append_str(order_tlv, &order_off, TAG_SZ, "1");
    tlv_append_bool(order_tlv, &order_off, TAG_REDUCE_ONLY, false);
    tlv_append_field(order_tlv, &order_off, TAG_LIMIT_SPEC, order_inner, order_inner_off);

    /* only TAG_MODIFY_ORDER, no TAG_OID */
    uint8_t modify_inner[512];
    size_t  modify_off = 0;
    tlv_append_field(modify_inner, &modify_off, TAG_MODIFY_ORDER, order_tlv, order_off);

    uint8_t buf[512];
    size_t  offset = 0;
    tlv_append_field(buf, &offset, TAG_MODIFY_REQUEST, modify_inner, modify_off);

    s_bulk_modify     result = {0};
    s_bulk_modify_ctx ctx    = {.bulk_modify = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_modify(&payload, &ctx));
}

static void test_parse_missing_order_in_modify_request_fails(void **state) {
    (void) state;
    /* Build a modify_request with only TAG_OID, no TAG_ORDER */
    uint8_t modify_inner[32];
    size_t  modify_off = 0;
    tlv_append_uint(modify_inner, &modify_off, TAG_OID, 42ULL);

    uint8_t buf[64];
    size_t  offset = 0;
    tlv_append_field(buf, &offset, TAG_MODIFY_REQUEST, modify_inner, modify_off);

    s_bulk_modify     result = {0};
    s_bulk_modify_ctx ctx    = {.bulk_modify = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_modify(&payload, &ctx));
}

static void test_parse_truncated_payload_fails(void **state) {
    (void) state;
    uint8_t  truncated[] = {TAG_MODIFY_REQUEST};
    buffer_t payload     = make_buffer(truncated, sizeof(truncated));

    s_bulk_modify     result = {0};
    s_bulk_modify_ctx ctx    = {.bulk_modify = &result};

    assert_false(parse_bulk_modify(&payload, &ctx));
}

/* ─── serialization tests ────────────────────────────────────────────────── */

static void test_serialize_single_modify(void **state) {
    (void) state;
    s_bulk_modify bm = {0};
    bm.modify_count = 1;
    bm.modifies[0].oid = 42ULL;
    bm.modifies[0].order.asset       = TEST_ASSET_ID;
    bm.modifies[0].order.is_buy      = true;
    bm.modifies[0].order.reduce_only = false;
    bm.modifies[0].order.order_type  = ORDER_TYPE_LIMIT;
    strncpy(bm.modifies[0].order.limit_px, "50000", sizeof(bm.modifies[0].order.limit_px) - 1);
    strncpy(bm.modifies[0].order.sz, "1", sizeof(bm.modifies[0].order.sz) - 1);
    bm.modifies[0].order.limit.tif = TIF_GTC;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(bulk_modify_serialize(&bm, &cmp));

    /* Decode and verify: fixmap{2} "type"->"batchModify" "modifies"->array[1] */
    cmp_ctx_t r = make_reader(&sb);
    uint32_t map_sz;
    assert_true(cmp_read_map(&r, &map_sz));
    assert_int_equal(map_sz, 2);

    assert_cmp_str(&r, "type");
    assert_cmp_str(&r, "batchModify");

    assert_cmp_str(&r, "modifies");
    uint32_t arr_sz;
    assert_true(cmp_read_array(&r, &arr_sz));
    assert_int_equal(arr_sz, 1);

    /* First modify: fixmap{2} "oid"->42 "order"->{...} */
    uint32_t mod_sz;
    assert_true(cmp_read_map(&r, &mod_sz));
    assert_int_equal(mod_sz, 2);
    assert_cmp_str(&r, "oid");
    uint64_t oid;
    assert_true(cmp_read_uinteger(&r, &oid));
    assert_int_equal(oid, 42ULL);
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_parse_single_modify, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_max_modifies, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_too_many_modifies_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_modify_request_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_oid_in_modify_request_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_order_in_modify_request_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_truncated_payload_fails, setup, teardown),
        cmocka_unit_test(test_serialize_single_modify),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
