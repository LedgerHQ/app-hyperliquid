#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "buffer.h"
#include "user_set_abstraction.h"
#include "tlv_helpers.h"

/* TAG values matching the parser definitions */
#define TAG_SIGNATURE_CHAIN_ID 0x23
#define TAG_ABSTRACTION        0xdf

/* ─── helpers ────────────────────────────────────────────────────────────── */

static size_t build_user_set_abstraction(uint8_t *buf,
                                          uint64_t chain_id,
                                          uint8_t  abstraction) {
    size_t offset = 0;
    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, chain_id);
    tlv_append_uint(buf, &offset, TAG_ABSTRACTION, abstraction);
    return offset;
}

/* ─── happy-path parse tests ─────────────────────────────────────────────── */

static void test_parse_unified_account(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_user_set_abstraction(buf, 42161, ABSTRACTION_UNIFIED_ACCOUNT);

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_user_set_abstraction(&payload, &ctx));

    assert_int_equal(result.signature_chain_id, 42161);
    assert_int_equal(result.abstraction, ABSTRACTION_UNIFIED_ACCOUNT);
}

static void test_parse_disabled(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_user_set_abstraction(buf, 1, ABSTRACTION_DISABLED);

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_user_set_abstraction(&payload, &ctx));

    assert_int_equal(result.abstraction, ABSTRACTION_DISABLED);
}

static void test_parse_portfolio_margin(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_user_set_abstraction(buf, 1, ABSTRACTION_PORTFOLIO_MARGIN);

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_user_set_abstraction(&payload, &ctx));

    assert_int_equal(result.abstraction, ABSTRACTION_PORTFOLIO_MARGIN);
}

/* ─── missing-field parse tests ──────────────────────────────────────────── */

static void test_parse_missing_chain_id_fails(void **state) {
    (void) state;
    uint8_t buf[16];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_ABSTRACTION, ABSTRACTION_UNIFIED_ACCOUNT);

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_user_set_abstraction(&payload, &ctx));
}

static void test_parse_missing_abstraction_fails(void **state) {
    (void) state;
    uint8_t buf[16];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 42161);

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_user_set_abstraction(&payload, &ctx));
}

static void test_parse_empty_payload_fails(void **state) {
    (void) state;
    uint8_t  empty[1] = {0};
    buffer_t payload  = make_buffer(empty, 0);

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    assert_false(parse_user_set_abstraction(&payload, &ctx));
}

/* ─── invalid-value parse tests ──────────────────────────────────────────── */

static void test_parse_invalid_abstraction_0x03_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_user_set_abstraction(buf, 42161, 0x03);

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_user_set_abstraction(&payload, &ctx));
}

static void test_parse_invalid_abstraction_0xff_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  len = build_user_set_abstraction(buf, 42161, 0xFF);

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_user_set_abstraction(&payload, &ctx));
}

/* ─── malformed TLV tests ────────────────────────────────────────────────── */

static void test_parse_duplicate_tag_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 42161);
    tlv_append_uint(buf, &offset, TAG_ABSTRACTION, ABSTRACTION_UNIFIED_ACCOUNT);
    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 1); /* duplicate */

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_user_set_abstraction(&payload, &ctx));
}

static void test_parse_truncated_payload_fails(void **state) {
    (void) state;
    uint8_t  truncated[] = {TAG_SIGNATURE_CHAIN_ID}; /* tag only, no length/value */
    buffer_t payload     = make_buffer(truncated, sizeof(truncated));

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    assert_false(parse_user_set_abstraction(&payload, &ctx));
}

static void test_parse_oversized_length_field_fails(void **state) {
    (void) state;
    /* tag=TAG_SIGNATURE_CHAIN_ID, length=50 (claims 50 bytes), only 3 follow */
    uint8_t  oversized[] = {TAG_SIGNATURE_CHAIN_ID, 0x32, 0x01, 0x02, 0x03};
    buffer_t payload     = make_buffer(oversized, sizeof(oversized));

    s_user_set_abstraction     result = {0};
    s_user_set_abstraction_ctx ctx    = {.user_set_abstraction = &result};

    assert_false(parse_user_set_abstraction(&payload, &ctx));
}

/* ─── get_abstraction_string tests ───────────────────────────────────────── */

static void test_get_abstraction_string_disabled(void **state) {
    (void) state;
    s_user_set_abstraction action = {.abstraction = ABSTRACTION_DISABLED};
    assert_string_equal(get_abstraction_string(&action), "disabled");
}

static void test_get_abstraction_string_unified_account(void **state) {
    (void) state;
    s_user_set_abstraction action = {.abstraction = ABSTRACTION_UNIFIED_ACCOUNT};
    assert_string_equal(get_abstraction_string(&action), "unifiedAccount");
}

static void test_get_abstraction_string_portfolio_margin(void **state) {
    (void) state;
    s_user_set_abstraction action = {.abstraction = ABSTRACTION_PORTFOLIO_MARGIN};
    assert_string_equal(get_abstraction_string(&action), "portfolioMargin");
}

static void test_get_abstraction_string_invalid_returns_null(void **state) {
    (void) state;
    s_user_set_abstraction action = {.abstraction = (e_abstraction) 0xFF};
    assert_null(get_abstraction_string(&action));
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_parse_unified_account),
        cmocka_unit_test(test_parse_disabled),
        cmocka_unit_test(test_parse_portfolio_margin),
        cmocka_unit_test(test_parse_missing_chain_id_fails),
        cmocka_unit_test(test_parse_missing_abstraction_fails),
        cmocka_unit_test(test_parse_empty_payload_fails),
        cmocka_unit_test(test_parse_invalid_abstraction_0x03_fails),
        cmocka_unit_test(test_parse_invalid_abstraction_0xff_fails),
        cmocka_unit_test(test_parse_duplicate_tag_fails),
        cmocka_unit_test(test_parse_truncated_payload_fails),
        cmocka_unit_test(test_parse_oversized_length_field_fails),
        cmocka_unit_test(test_get_abstraction_string_disabled),
        cmocka_unit_test(test_get_abstraction_string_unified_account),
        cmocka_unit_test(test_get_abstraction_string_portfolio_margin),
        cmocka_unit_test(test_get_abstraction_string_invalid_returns_null),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
