#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include <cmocka.h>

#include "buffer.h"
#include "hl_context.h"
#include "update_isolated_margin.h"
#include "tlv_helpers.h"
#include "ser_helpers.h"

/* TAG values matching the parser definitions */
#define TAG_ASSET  0xd1
#define TAG_IS_BUY 0xe2
#define TAG_NTLI   0xd6

#define TEST_ASSET_ID 7U

/* ─── context setup ──────────────────────────────────────────────────────── */

static int setup(void **state) {
    (void) state;
    ctx_reset();
    s_action_metadata m = {0};
    m.op_type  = OP_TYPE_UPDATE_MARGIN;
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

static size_t build_update_isolated_margin(uint8_t *buf,
                                            uint32_t asset,
                                            bool     is_buy,
                                            int64_t  ntli) {
    size_t offset = 0;
    tlv_append_uint(buf, &offset, TAG_ASSET, asset);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, is_buy);
    tlv_append_int64_be(buf, &offset, TAG_NTLI, ntli);
    return offset;
}

/* ─── parse tests ────────────────────────────────────────────────────────── */

static void test_parse_valid_positive_ntli(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_update_isolated_margin(buf, TEST_ASSET_ID, true, 1000000LL);

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_update_isolated_margin(&payload, &ctx));

    assert_int_equal(result.asset, TEST_ASSET_ID);
    assert_true(result.is_buy);
    assert_int_equal(result.ntli, 1000000LL);
}

static void test_parse_valid_negative_ntli(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_update_isolated_margin(buf, TEST_ASSET_ID, false, -500000LL);

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_update_isolated_margin(&payload, &ctx));

    assert_false(result.is_buy);
    assert_int_equal(result.ntli, -500000LL);
}

static void test_parse_asset_mismatch_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    /* Asset 99 != TEST_ASSET_ID */
    size_t len = build_update_isolated_margin(buf, 99, true, 100LL);

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_update_isolated_margin(&payload, &ctx));
}

static void test_parse_missing_asset_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  offset = 0;
    tlv_append_bool(buf, &offset, TAG_IS_BUY, true);
    tlv_append_int64_be(buf, &offset, TAG_NTLI, 100LL);

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_update_isolated_margin(&payload, &ctx));
}

static void test_parse_missing_is_buy_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_int64_be(buf, &offset, TAG_NTLI, 100LL);

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_update_isolated_margin(&payload, &ctx));
}

static void test_parse_missing_ntli_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_ASSET, TEST_ASSET_ID);
    tlv_append_bool(buf, &offset, TAG_IS_BUY, true);

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_update_isolated_margin(&payload, &ctx));
}

/* ─── zero and boundary value tests ─────────────────────────────────────── */

static void test_parse_zero_ntli_succeeds(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_update_isolated_margin(buf, TEST_ASSET_ID, true, 0LL);

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_update_isolated_margin(&payload, &ctx));
    assert_int_equal(result.ntli, 0LL);
}

static void test_parse_int64_min_ntli_succeeds(void **state) {
    (void) state;
    /* INT64_MIN is the most dangerous value for signed negation; parser must accept it */
    uint8_t buf[64];
    size_t  len = build_update_isolated_margin(buf, TEST_ASSET_ID, false, INT64_MIN);

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_update_isolated_margin(&payload, &ctx));
    assert_int_equal(result.ntli, INT64_MIN);
}

static void test_parse_int64_max_ntli_succeeds(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_update_isolated_margin(buf, TEST_ASSET_ID, true, INT64_MAX);

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_update_isolated_margin(&payload, &ctx));
    assert_int_equal(result.ntli, INT64_MAX);
}

/* ─── malformed / truncated TLV tests ───────────────────────────────────── */

static void test_parse_truncated_payload_fails(void **state) {
    (void) state;
    /* Single tag byte — no length or value follows */
    uint8_t  truncated[] = {TAG_ASSET};
    buffer_t payload     = make_buffer(truncated, sizeof(truncated));

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    assert_false(parse_update_isolated_margin(&payload, &ctx));
}

static void test_parse_oversized_length_field_fails(void **state) {
    (void) state;
    /* tag=TAG_ASSET, length=50 (claims 50 bytes), only 3 bytes follow */
    uint8_t  oversized[] = {TAG_ASSET, 0x32, 0x01, 0x02, 0x03};
    buffer_t payload     = make_buffer(oversized, sizeof(oversized));

    s_update_isolated_margin     result = {0};
    s_update_isolated_margin_ctx ctx    = {.update_isolated_margin = &result};

    assert_false(parse_update_isolated_margin(&payload, &ctx));
}

/* ─── serialization tests ────────────────────────────────────────────────── */

static void test_serialize_positive_ntli(void **state) {
    (void) state;
    s_update_isolated_margin uim = {
        .asset  = TEST_ASSET_ID,
        .is_buy = true,
        .ntli   = 1000000LL,
    };

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(update_isolated_margin_serialize(&uim, &cmp));

    /* fixmap(4) = 0x84 */
    assert_int_equal(sb.data[0], 0x84);
    /* Verify all key names and the type string value */
    ASSERT_SERIALIZED_STR(sb, "type");
    ASSERT_SERIALIZED_STR(sb, "updateIsolatedMargin");
    ASSERT_SERIALIZED_STR(sb, "asset");
    ASSERT_SERIALIZED_STR(sb, "isBuy");
    ASSERT_SERIALIZED_STR(sb, "ntli");
}

static void test_serialize_negative_ntli(void **state) {
    (void) state;
    s_update_isolated_margin uim = {
        .asset  = TEST_ASSET_ID,
        .is_buy = false,
        .ntli   = -1000000LL,
    };

    ser_buf_t sb  = {0};
    cmp_ctx_t cmp = make_writer(&sb);
    assert_true(update_isolated_margin_serialize(&uim, &cmp));

    /* fixmap(4) = 0x84; same key names for negative ntli */
    assert_int_equal(sb.data[0], 0x84);
    ASSERT_SERIALIZED_STR(sb, "updateIsolatedMargin");
    ASSERT_SERIALIZED_STR(sb, "ntli");
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_parse_valid_positive_ntli, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_valid_negative_ntli, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_asset_mismatch_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_asset_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_is_buy_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_ntli_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_zero_ntli_succeeds, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_int64_min_ntli_succeeds, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_int64_max_ntli_succeeds, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_truncated_payload_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_oversized_length_field_fails, setup, teardown),
        cmocka_unit_test(test_serialize_positive_ntli),
        cmocka_unit_test(test_serialize_negative_ntli),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
