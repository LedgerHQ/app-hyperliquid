#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "buffer.h"
#include "hl_context.h"
#include "bulk_cancel.h"
#include "tlv_helpers.h"
#include "ser_helpers.h"

/* TAG values matching the parser definitions */
#define TAG_CANCEL_REQUEST 0xd9
#define TAG_ASSET          0xd1
#define TAG_OID            0xdc

#define TEST_ASSET_ID 10U

/* ─── context setup ──────────────────────────────────────────────────────── */

static int setup(void **state) {
    (void) state;
    ctx_reset();
    s_action_metadata m = {0};
    m.op_type  = OP_TYPE_CANCEL;
    m.asset_id = TEST_ASSET_ID;
    m.network  = NETWORK_MAINNET;
    strncpy(m.asset_ticker, "BNB", sizeof(m.asset_ticker) - 1);
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
 * Builds a cancel_request TLV (inner nested struct) into buf at *offset.
 */
static void append_cancel_request(uint8_t *outer,
                                   size_t  *outer_offset,
                                   uint32_t asset,
                                   uint64_t oid) {
    uint8_t inner[32];
    size_t  inner_offset = 0;
    tlv_append_uint(inner, &inner_offset, TAG_ASSET, asset);
    tlv_append_uint(inner, &inner_offset, TAG_OID, oid);
    /* Wrap in cancel_request tag */
    tlv_append_field(outer, outer_offset, TAG_CANCEL_REQUEST, inner, inner_offset);
}

/* ─── parse tests ────────────────────────────────────────────────────────── */

static void test_parse_single_cancel(void **state) {
    (void) state;
    uint8_t buf[128];
    size_t  offset = 0;
    append_cancel_request(buf, &offset, TEST_ASSET_ID, 100ULL);

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_true(parse_bulk_cancel(&payload, &ctx));

    assert_int_equal(result.cancel_count, 1);
    assert_int_equal(result.cancels[0].asset, TEST_ASSET_ID);
    assert_int_equal(result.cancels[0].oid, 100ULL);
}

static void test_parse_max_cancels(void **state) {
    (void) state;
    uint8_t buf[256];
    size_t  offset = 0;

    for (uint64_t i = 0; i < BULK_MAX_SIZE; ++i) {
        append_cancel_request(buf, &offset, TEST_ASSET_ID, i + 1);
    }

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_true(parse_bulk_cancel(&payload, &ctx));

    assert_int_equal(result.cancel_count, BULK_MAX_SIZE);
    for (uint8_t i = 0; i < BULK_MAX_SIZE; ++i) {
        assert_int_equal(result.cancels[i].oid, (uint64_t)(i + 1));
    }
}

static void test_parse_too_many_cancels_fails(void **state) {
    (void) state;
    uint8_t buf[512];
    size_t  offset = 0;

    /* BULK_MAX_SIZE + 1 cancel requests must overflow and fail */
    for (uint64_t i = 0; i < (uint64_t)(BULK_MAX_SIZE + 1); ++i) {
        append_cancel_request(buf, &offset, TEST_ASSET_ID, i + 1);
    }

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_cancel(&payload, &ctx));
}

static void test_parse_asset_mismatch_fails(void **state) {
    (void) state;
    uint8_t buf[128];
    size_t  offset = 0;
    /* Asset 99 != TEST_ASSET_ID */
    append_cancel_request(buf, &offset, 99, 200ULL);

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_bulk_cancel(&payload, &ctx));
}

static void test_parse_missing_cancel_request_fails(void **state) {
    (void) state;
    /* Empty payload – TAG_CANCEL_REQUEST is required */
    uint8_t  empty[1] = {0};
    buffer_t payload  = make_buffer(empty, 0);

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    assert_false(parse_bulk_cancel(&payload, &ctx));
}

static void test_parse_cancel_missing_oid_fails(void **state) {
    (void) state;
    /* cancel_request with only asset tag, no oid */
    uint8_t inner[16];
    size_t  inner_off = 0;
    tlv_append_uint(inner, &inner_off, TAG_ASSET, TEST_ASSET_ID);

    uint8_t outer[64];
    size_t  outer_off = 0;
    tlv_append_field(outer, &outer_off, TAG_CANCEL_REQUEST, inner, inner_off);

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    buffer_t payload = make_buffer(outer, outer_off);
    assert_false(parse_bulk_cancel(&payload, &ctx));
}

/* ─── zero value tests ───────────────────────────────────────────────────── */

static void test_parse_zero_oid_succeeds(void **state) {
    (void) state;
    /* oid=0 is a valid order ID; the parser must accept it */
    uint8_t buf[128];
    size_t  offset = 0;
    append_cancel_request(buf, &offset, TEST_ASSET_ID, 0ULL);

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_true(parse_bulk_cancel(&payload, &ctx));

    assert_int_equal(result.cancel_count, 1);
    assert_int_equal(result.cancels[0].oid, 0ULL);
}

/* ─── malformed / truncated TLV tests ───────────────────────────────────── */

static void test_parse_truncated_cancel_request_fails(void **state) {
    (void) state;
    /* Outer tag present but inner TLV is truncated (only 1 byte of value) */
    uint8_t inner[] = {TAG_ASSET};  /* tag only, no length/value */
    uint8_t outer[64];
    size_t  outer_off = 0;
    tlv_append_field(outer, &outer_off, TAG_CANCEL_REQUEST, inner, sizeof(inner));

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    buffer_t payload = make_buffer(outer, outer_off);
    assert_false(parse_bulk_cancel(&payload, &ctx));
}

static void test_parse_truncated_outer_payload_fails(void **state) {
    (void) state;
    /* Single tag byte — no length or value follows */
    uint8_t  truncated[] = {TAG_CANCEL_REQUEST};
    buffer_t payload     = make_buffer(truncated, sizeof(truncated));

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    assert_false(parse_bulk_cancel(&payload, &ctx));
}

static void test_parse_oversized_length_field_fails(void **state) {
    (void) state;
    /* tag=TAG_CANCEL_REQUEST, length=100 (claims 100 bytes), only 3 bytes follow */
    uint8_t  oversized[] = {TAG_CANCEL_REQUEST, 0x64, 0x01, 0x02, 0x03};
    buffer_t payload     = make_buffer(oversized, sizeof(oversized));

    s_bulk_cancel     result = {0};
    s_bulk_cancel_ctx ctx    = {.bulk_cancel = &result};

    assert_false(parse_bulk_cancel(&payload, &ctx));
}

/* ─── serialization tests ────────────────────────────────────────────────── */

static void test_serialize_single_cancel(void **state) {
    (void) state;
    s_bulk_cancel bc = {0};
    bc.cancel_count  = 1;
    bc.cancels[0].asset = TEST_ASSET_ID;
    bc.cancels[0].oid   = 42ULL;

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(bulk_cancel_serialize(&bc, &cmp));

    /* fixmap(2) = 0x82 */
    assert_int_equal(sb.data[0], 0x82);
    /* Verify all key names and the type string value */
    ASSERT_SERIALIZED_STR(sb, "type");
    ASSERT_SERIALIZED_STR(sb, "cancel");
    ASSERT_SERIALIZED_STR(sb, "cancels");
    ASSERT_SERIALIZED_STR(sb, "a");
    ASSERT_SERIALIZED_STR(sb, "o");
}

static void test_serialize_multiple_cancels(void **state) {
    (void) state;
    s_bulk_cancel bc = {0};
    bc.cancel_count  = 3;
    for (uint8_t i = 0; i < 3; ++i) {
        bc.cancels[i].asset = TEST_ASSET_ID;
        bc.cancels[i].oid   = (uint64_t)(i + 1);
    }

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(bulk_cancel_serialize(&bc, &cmp));

    /* fixmap(2); same key names regardless of count */
    assert_int_equal(sb.data[0], 0x82);
    ASSERT_SERIALIZED_STR(sb, "cancel");
    ASSERT_SERIALIZED_STR(sb, "cancels");
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_parse_single_cancel, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_max_cancels, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_too_many_cancels_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_asset_mismatch_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_cancel_request_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_cancel_missing_oid_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_zero_oid_succeeds, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_truncated_cancel_request_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_truncated_outer_payload_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_oversized_length_field_fails, setup, teardown),
        cmocka_unit_test(test_serialize_single_cancel),
        cmocka_unit_test(test_serialize_multiple_cancels),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
