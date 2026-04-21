#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "buffer.h"
#include "approve_builder_fee.h"
#include "tlv_helpers.h"
#include "constants.h"

/* TAG values matching the parser definitions */
#define TAG_SIGNATURE_CHAIN_ID 0x23
#define TAG_MAX_FEE_RATE       0xb0
#define TAG_BUILDER_ADDR       0xd3

/* ─── helpers ────────────────────────────────────────────────────────────── */

/**
 * Builds a well-formed approve_builder_fee TLV payload.
 * chain_id=1337, max_fee_rate="0.1", builder=20 bytes {0x01..0x14}
 */
static size_t build_valid_approve_builder_fee(uint8_t *buf, size_t bufsize) {
    static const uint8_t builder_addr[ADDRESS_LENGTH] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
        0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
    };
    (void) bufsize;
    size_t offset = 0;
    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 1337);
    tlv_append_str(buf, &offset, TAG_MAX_FEE_RATE, "0.1");
    tlv_append_bytes(buf, &offset, TAG_BUILDER_ADDR, builder_addr, sizeof(builder_addr));
    return offset;
}

/* ─── tests ──────────────────────────────────────────────────────────────── */

static void test_parse_valid_approve_builder_fee(void **state) {
    (void) state;
    uint8_t buf[128];
    size_t  len = build_valid_approve_builder_fee(buf, sizeof(buf));

    s_approve_builder_fee         result = {0};
    s_approve_builder_fee_ctx     ctx    = {0};
    ctx.approve_builder_fee = &result;

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_approve_builder_fee(&payload, &ctx));

    assert_int_equal(result.signature_chain_id, 1337);
    assert_string_equal(result.max_fee_rate, "0.1");

    /* Verify builder address bytes */
    for (int i = 0; i < ADDRESS_LENGTH; ++i) {
        assert_int_equal(result.builder[i], i + 1);
    }
}

static void test_parse_null_payload_fails(void **state) {
    (void) state;
    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    /* The TLV parser itself receives NULL through the buffer wrapper */
    uint8_t  empty[1] = {0};
    buffer_t payload  = make_buffer(empty, 0);
    /* An empty payload has no tags → required tags are absent → must fail */
    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

static void test_parse_missing_chain_id_fails(void **state) {
    (void) state;
    static const uint8_t builder_addr[ADDRESS_LENGTH] = {0};
    uint8_t buf[128];
    size_t  offset = 0;

    /* Omit TAG_SIGNATURE_CHAIN_ID */
    tlv_append_str(buf, &offset, TAG_MAX_FEE_RATE, "0.1");
    tlv_append_bytes(buf, &offset, TAG_BUILDER_ADDR, builder_addr, sizeof(builder_addr));

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

static void test_parse_missing_max_fee_rate_fails(void **state) {
    (void) state;
    static const uint8_t builder_addr[ADDRESS_LENGTH] = {0};
    uint8_t buf[128];
    size_t  offset = 0;

    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 1337);
    /* Omit TAG_MAX_FEE_RATE */
    tlv_append_bytes(buf, &offset, TAG_BUILDER_ADDR, builder_addr, sizeof(builder_addr));

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

static void test_parse_missing_builder_addr_fails(void **state) {
    (void) state;
    uint8_t buf[64];
    size_t  offset = 0;

    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 1337);
    tlv_append_str(buf, &offset, TAG_MAX_FEE_RATE, "0.001");
    /* Omit TAG_BUILDER_ADDR */

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

static void test_parse_duplicate_tag_fails(void **state) {
    (void) state;
    static const uint8_t builder_addr[ADDRESS_LENGTH] = {0};
    uint8_t buf[256];
    size_t  offset = 0;

    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 1337);
    tlv_append_str(buf, &offset, TAG_MAX_FEE_RATE, "0.1");
    /* Duplicate chain_id tag */
    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 42);
    tlv_append_bytes(buf, &offset, TAG_BUILDER_ADDR, builder_addr, sizeof(builder_addr));

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

static void test_parse_builder_wrong_size_fails(void **state) {
    (void) state;
    /* Only 10 bytes instead of the required ADDRESS_LENGTH (20) */
    static const uint8_t short_builder[10] = {0};
    uint8_t buf[128];
    size_t  offset = 0;

    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 1);
    tlv_append_str(buf, &offset, TAG_MAX_FEE_RATE, "1");
    tlv_append_bytes(buf, &offset, TAG_BUILDER_ADDR, short_builder, sizeof(short_builder));

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

static void test_parse_max_fee_rate_empty_fails(void **state) {
    (void) state;
    /* min_length for get_string_from_tlv_data is 1 → empty string must fail */
    static const uint8_t builder_addr[ADDRESS_LENGTH] = {0};
    uint8_t buf[128];
    size_t  offset = 0;

    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 1);
    tlv_append_str(buf, &offset, TAG_MAX_FEE_RATE, "");
    tlv_append_bytes(buf, &offset, TAG_BUILDER_ADDR, builder_addr, sizeof(builder_addr));

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

static void test_parse_max_length_fee_rate_succeeds(void **state) {
    (void) state;
    /* NUMERIC_STRING_LENGTH = 21; buffer is [22]; a 21-char string exactly fits */
    static const uint8_t builder_addr[ADDRESS_LENGTH] = {0};
    char max_str[NUMERIC_STRING_LENGTH + 1];
    memset(max_str, '9', NUMERIC_STRING_LENGTH);
    max_str[NUMERIC_STRING_LENGTH] = '\0';

    uint8_t buf[256];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 1);
    tlv_append_str(buf, &offset, TAG_MAX_FEE_RATE, max_str);
    tlv_append_bytes(buf, &offset, TAG_BUILDER_ADDR, builder_addr, sizeof(builder_addr));

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    buffer_t payload = make_buffer(buf, offset);
    assert_true(parse_approve_builder_fee(&payload, &ctx));
    assert_string_equal(result.max_fee_rate, max_str);
}

static void test_parse_too_long_fee_rate_fails(void **state) {
    (void) state;
    /* 22 chars + NUL = 23 bytes needed, buffer is only 22 → must fail */
    static const uint8_t builder_addr[ADDRESS_LENGTH] = {0};
    char over_str[NUMERIC_STRING_LENGTH + 2 + 1];
    memset(over_str, '9', NUMERIC_STRING_LENGTH + 2);
    over_str[NUMERIC_STRING_LENGTH + 2] = '\0';

    uint8_t buf[256];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_SIGNATURE_CHAIN_ID, 1);
    tlv_append_str(buf, &offset, TAG_MAX_FEE_RATE, over_str);
    tlv_append_bytes(buf, &offset, TAG_BUILDER_ADDR, builder_addr, sizeof(builder_addr));

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

static void test_parse_truncated_payload_fails(void **state) {
    (void) state;
    /* Single tag byte — no length or value follows */
    uint8_t  truncated[] = {TAG_SIGNATURE_CHAIN_ID};
    buffer_t payload     = make_buffer(truncated, sizeof(truncated));

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

static void test_parse_oversized_length_field_fails(void **state) {
    (void) state;
    /* tag=TAG_SIGNATURE_CHAIN_ID, length=50 (claims 50 bytes), only 3 follow */
    uint8_t  oversized[] = {TAG_SIGNATURE_CHAIN_ID, 0x32, 0x01, 0x02, 0x03};
    buffer_t payload     = make_buffer(oversized, sizeof(oversized));

    s_approve_builder_fee     result = {0};
    s_approve_builder_fee_ctx ctx    = {0};
    ctx.approve_builder_fee = &result;

    assert_false(parse_approve_builder_fee(&payload, &ctx));
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_parse_valid_approve_builder_fee),
        cmocka_unit_test(test_parse_null_payload_fails),
        cmocka_unit_test(test_parse_missing_chain_id_fails),
        cmocka_unit_test(test_parse_missing_max_fee_rate_fails),
        cmocka_unit_test(test_parse_missing_builder_addr_fails),
        cmocka_unit_test(test_parse_duplicate_tag_fails),
        cmocka_unit_test(test_parse_builder_wrong_size_fails),
        cmocka_unit_test(test_parse_max_fee_rate_empty_fails),
        cmocka_unit_test(test_parse_max_length_fee_rate_succeeds),
        cmocka_unit_test(test_parse_too_long_fee_rate_fails),
        cmocka_unit_test(test_parse_truncated_payload_fails),
        cmocka_unit_test(test_parse_oversized_length_field_fails),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
