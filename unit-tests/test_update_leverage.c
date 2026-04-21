#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "buffer.h"
#include "hl_context.h"
#include "update_leverage.h"
#include "tlv_helpers.h"
#include "ser_helpers.h"

/* TAG values matching the parser definitions */
#define TAG_ASSET    0xd1
#define TAG_IS_CROSS 0xde
#define TAG_LEVERAGE 0xed

#define TEST_ASSET_ID 42U
#define TEST_LEVERAGE 10U

/* ─── context setup ──────────────────────────────────────────────────────── */

static int setup(void **state) {
    (void) state;
    ctx_reset();
    s_action_metadata m = {0};
    m.op_type  = OP_TYPE_UPDATE_LEVERAGE;
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

static size_t build_update_leverage(uint8_t *buf,
                                     uint32_t asset,
                                     bool     is_cross,
                                     uint32_t leverage) {
    size_t offset = 0;
    tlv_append_uint(buf, &offset, TAG_ASSET, asset);
    tlv_append_bool(buf, &offset, TAG_IS_CROSS, is_cross);
    tlv_append_uint(buf, &offset, TAG_LEVERAGE, leverage);
    return offset;
}

/* ─── parse tests ────────────────────────────────────────────────────────── */

static void test_parse_valid_cross(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_update_leverage(buf, TEST_ASSET_ID, true, TEST_LEVERAGE);

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_update_leverage(&payload, &ctx));

    assert_int_equal(result.asset, TEST_ASSET_ID);
    assert_true(result.is_cross);
    assert_int_equal(result.leverage, TEST_LEVERAGE);
}

static void test_parse_valid_isolated(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_update_leverage(buf, TEST_ASSET_ID, false, 5);

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_update_leverage(&payload, &ctx));

    assert_false(result.is_cross);
    assert_int_equal(result.leverage, 5);
}

static void test_parse_asset_mismatch_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    /* Asset 99 does not match the metadata asset_id (TEST_ASSET_ID = 42) */
    size_t len = build_update_leverage(buf, 99, true, TEST_LEVERAGE);

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_update_leverage(&payload, &ctx));
}

static void test_parse_missing_asset_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  offset = 0;
    tlv_append_bool(buf, &offset, TAG_IS_CROSS, true);
    tlv_append_uint(buf, &offset, TAG_LEVERAGE, TEST_LEVERAGE);

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_update_leverage(&payload, &ctx));
}

static void test_parse_missing_is_cross_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_uint(buf, &offset, TAG_LEVERAGE, TEST_LEVERAGE);

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_update_leverage(&payload, &ctx));
}

static void test_parse_missing_leverage_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_bool(buf, &offset, TAG_IS_CROSS, false);

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_update_leverage(&payload, &ctx));
}

static void test_parse_empty_payload_fails(void **state) {
    (void) state;
    uint8_t  empty[1] = {0};
    buffer_t payload  = make_buffer(empty, 0);

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    assert_false(parse_update_leverage(&payload, &ctx));
}

/* ─── zero value tests ───────────────────────────────────────────────────── */

static void test_parse_zero_leverage_succeeds(void **state) {
    (void) state;
    /* Leverage=0 has no semantic validation in the parser; it must parse cleanly */
    uint8_t buf[64];
    size_t  len = build_update_leverage(buf, TEST_ASSET_ID, true, 0);

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_update_leverage(&payload, &ctx));
    assert_int_equal(result.leverage, 0);
}

/* ─── malformed / truncated TLV tests ───────────────────────────────────── */

static void test_parse_truncated_payload_fails(void **state) {
    (void) state;
    /* Single tag byte — no length or value follows */
    uint8_t  truncated[] = {TAG_ASSET};
    buffer_t payload     = make_buffer(truncated, sizeof(truncated));

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    assert_false(parse_update_leverage(&payload, &ctx));
}

static void test_parse_oversized_length_field_fails(void **state) {
    (void) state;
    /* tag=TAG_ASSET, length=50 (claims 50 bytes), only 3 bytes follow */
    uint8_t  oversized[] = {TAG_ASSET, 0x32, 0x01, 0x02, 0x03};
    buffer_t payload     = make_buffer(oversized, sizeof(oversized));

    s_update_leverage     result = {0};
    s_update_leverage_ctx ctx    = {.update_leverage = &result};

    assert_false(parse_update_leverage(&payload, &ctx));
}

/* ─── serialization tests ────────────────────────────────────────────────── */

/* ─── serialization tests ────────────────────────────────────────────────── */

static void test_serialize_cross_leverage(void **state) {
    (void) state;
    s_update_leverage ul = {
        .asset    = TEST_ASSET_ID,
        .is_cross = true,
        .leverage = TEST_LEVERAGE,
    };

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(update_leverage_serialize(&ul, &cmp));

    /* fixmap(4) = 0x84 */
    assert_int_equal(sb.data[0], 0x84);
    /* Verify all key names and the type string value */
    ASSERT_SERIALIZED_STR(sb, "type");
    ASSERT_SERIALIZED_STR(sb, "updateLeverage");
    ASSERT_SERIALIZED_STR(sb, "asset");
    ASSERT_SERIALIZED_STR(sb, "isCross");
    ASSERT_SERIALIZED_STR(sb, "leverage");
}

static void test_serialize_isolated_leverage(void **state) {
    (void) state;
    s_update_leverage ul = {
        .asset    = TEST_ASSET_ID,
        .is_cross = false,
        .leverage = 50,
    };

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(update_leverage_serialize(&ul, &cmp));

    /* Still a fixmap of 4 entries; same key names */
    assert_int_equal(sb.data[0], 0x84);
    ASSERT_SERIALIZED_STR(sb, "updateLeverage");
    ASSERT_SERIALIZED_STR(sb, "isCross");
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_parse_valid_cross, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_valid_isolated, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_asset_mismatch_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_asset_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_is_cross_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_leverage_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_empty_payload_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_zero_leverage_succeeds, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_truncated_payload_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_oversized_length_field_fails, setup, teardown),
        cmocka_unit_test(test_serialize_cross_leverage),
        cmocka_unit_test(test_serialize_isolated_leverage),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
