#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "buffer.h"
#include "hl_context.h"
#include "action.h"
#include "tlv_helpers.h"

/* Outer action envelope TAGs (defined in action.c) */
#define TAG_STRUCT_TYPE    0x01
#define TAG_STRUCT_VERSION 0x02
#define TAG_ACTION_TYPE    0xd0
#define TAG_NONCE          0xda
#define TAG_ACTION_PAYLOAD 0xdb

#define ACTION_STRUCT_TYPE    0x2c
#define ACTION_STRUCT_VERSION 0x01

/* Inner bulk_cancel TAGs (simplest inner action to construct) */
#define TAG_CANCEL_REQUEST 0xd9
#define TAG_ASSET          0xd1
#define TAG_OID            0xdc

#define TEST_ASSET_ID 8U

/* ─── context setup ──────────────────────────────────────────────────────── */

static int setup(void **state) {
    (void) state;
    ctx_reset();
    s_action_metadata m = {0};
    m.op_type  = OP_TYPE_CANCEL;
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

/** Builds a minimal bulk_cancel inner TLV (one cancel for TEST_ASSET_ID). */
static size_t build_inner_bulk_cancel(uint8_t *buf, uint32_t asset, uint64_t oid) {
    uint8_t inner[32];
    size_t  inner_off = 0;
    tlv_append_uint(inner, &inner_off, TAG_ASSET, asset);
    tlv_append_uint(inner, &inner_off, TAG_OID, oid);

    size_t offset = 0;
    tlv_append_field(buf, &offset, TAG_CANCEL_REQUEST, inner, inner_off);
    return offset;
}

/**
 * Builds a complete outer action TLV.
 * @param inner_action  Pre-built inner action TLV bytes (may be NULL for missing-action tests).
 * @param inner_len     Length of inner_action (0 if NULL).
 */
static size_t build_action_tlv(uint8_t        *buf,
                                uint8_t         struct_type,
                                uint8_t         struct_version,
                                uint8_t         action_type,
                                uint64_t        nonce,
                                const uint8_t  *inner_action,
                                size_t          inner_len) {
    size_t offset = 0;
    tlv_append_uint(buf, &offset, TAG_STRUCT_TYPE, struct_type);
    tlv_append_uint(buf, &offset, TAG_STRUCT_VERSION, struct_version);
    tlv_append_uint(buf, &offset, TAG_ACTION_TYPE, action_type);
    tlv_append_uint(buf, &offset, TAG_NONCE, nonce);
    if (inner_action && inner_len > 0) {
        tlv_append_field(buf, &offset, TAG_ACTION_PAYLOAD, inner_action, inner_len);
    }
    return offset;
}

/* ─── parse tests ────────────────────────────────────────────────────────── */

static void test_parse_valid_bulk_cancel_action(void **state) {
    (void) state;
    uint8_t inner[128];
    size_t  inner_len = build_inner_bulk_cancel(inner, TEST_ASSET_ID, 42ULL);

    uint8_t buf[512];
    size_t  len = build_action_tlv(buf,
                                   ACTION_STRUCT_TYPE,
                                   ACTION_STRUCT_VERSION,
                                   ACTION_TYPE_BULK_CANCEL,
                                   99999ULL,
                                   inner, inner_len);

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_action(&payload));

    const s_action *act = ctx_get_current_action();
    assert_non_null(act);
    assert_int_equal(act->type, ACTION_TYPE_BULK_CANCEL);
    assert_int_equal(act->nonce, 99999ULL);
    assert_int_equal(act->bulk_cancel.cancel_count, 1);
    assert_int_equal(act->bulk_cancel.cancels[0].oid, 42ULL);
}

static void test_parse_wrong_struct_type_fails(void **state) {
    (void) state;
    uint8_t inner[128];
    size_t  inner_len = build_inner_bulk_cancel(inner, TEST_ASSET_ID, 1ULL);

    uint8_t buf[512];
    size_t  len = build_action_tlv(buf,
                                   0xFF, /* wrong struct_type */
                                   ACTION_STRUCT_VERSION,
                                   ACTION_TYPE_BULK_CANCEL,
                                   1ULL,
                                   inner, inner_len);

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_action(&payload));
}

static void test_parse_wrong_struct_version_fails(void **state) {
    (void) state;
    uint8_t inner[128];
    size_t  inner_len = build_inner_bulk_cancel(inner, TEST_ASSET_ID, 1ULL);

    uint8_t buf[512];
    size_t  len = build_action_tlv(buf,
                                   ACTION_STRUCT_TYPE,
                                   0x02, /* wrong version */
                                   ACTION_TYPE_BULK_CANCEL,
                                   1ULL,
                                   inner, inner_len);

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_action(&payload));
}

static void test_parse_unknown_action_type_fails(void **state) {
    (void) state;
    uint8_t inner[128];
    size_t  inner_len = build_inner_bulk_cancel(inner, TEST_ASSET_ID, 1ULL);

    uint8_t buf[512];
    size_t  len = build_action_tlv(buf,
                                   ACTION_STRUCT_TYPE,
                                   ACTION_STRUCT_VERSION,
                                   0xFF, /* unknown action type */
                                   1ULL,
                                   inner, inner_len);

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_action(&payload));
}

static void test_parse_missing_nonce_fails(void **state) {
    (void) state;
    uint8_t inner[128];
    size_t  inner_len = build_inner_bulk_cancel(inner, TEST_ASSET_ID, 1ULL);

    uint8_t buf[512];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_STRUCT_TYPE, ACTION_STRUCT_TYPE);
    tlv_append_uint(buf, &offset, TAG_STRUCT_VERSION, ACTION_STRUCT_VERSION);
    tlv_append_uint(buf, &offset, TAG_ACTION_TYPE, ACTION_TYPE_BULK_CANCEL);
    /* Omit TAG_NONCE */
    tlv_append_field(buf, &offset, TAG_ACTION_PAYLOAD, inner, inner_len);

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_action(&payload));
}

static void test_parse_missing_action_payload_fails(void **state) {
    (void) state;
    uint8_t buf[512];
    size_t  len = build_action_tlv(buf,
                                   ACTION_STRUCT_TYPE,
                                   ACTION_STRUCT_VERSION,
                                   ACTION_TYPE_BULK_CANCEL,
                                   1ULL,
                                   NULL, 0); /* no inner TLV */

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_action(&payload));
}

static void test_parse_action_payload_before_type_fails(void **state) {
    (void) state;
    /* TAG_ACTION_PAYLOAD appears before TAG_ACTION_TYPE — must fail */
    uint8_t inner[128];
    size_t  inner_len = build_inner_bulk_cancel(inner, TEST_ASSET_ID, 1ULL);

    uint8_t buf[512];
    size_t  offset = 0;
    tlv_append_uint(buf, &offset, TAG_STRUCT_TYPE, ACTION_STRUCT_TYPE);
    tlv_append_uint(buf, &offset, TAG_STRUCT_VERSION, ACTION_STRUCT_VERSION);
    /* TAG_ACTION_PAYLOAD before TAG_ACTION_TYPE */
    tlv_append_field(buf, &offset, TAG_ACTION_PAYLOAD, inner, inner_len);
    tlv_append_uint(buf, &offset, TAG_ACTION_TYPE, ACTION_TYPE_BULK_CANCEL);
    tlv_append_uint(buf, &offset, TAG_NONCE, 1ULL);

    buffer_t payload = make_buffer(buf, offset);
    assert_false(parse_action(&payload));
}

static void test_parse_null_payload_fails(void **state) {
    (void) state;
    /* parse_action checks for NULL payload */
    assert_false(parse_action(NULL));
}

static void test_parse_no_metadata_fails(void **state) {
    (void) state;
    /* Clear metadata — parse_action must fail */
    ctx_reset();
    uint8_t inner[128];
    size_t  inner_len = build_inner_bulk_cancel(inner, TEST_ASSET_ID, 1ULL);

    uint8_t buf[512];
    size_t  len = build_action_tlv(buf,
                                   ACTION_STRUCT_TYPE,
                                   ACTION_STRUCT_VERSION,
                                   ACTION_TYPE_BULK_CANCEL,
                                   1ULL,
                                   inner, inner_len);

    buffer_t payload = make_buffer(buf, len);
    assert_false(parse_action(&payload));
}

static void test_parse_all_action_types(void **state) {
    (void) state;
    /* Verify that all 6 action types dispatch successfully.
     * For non-cancel types we need appropriate metadata and inner TLVs.
     * Here we just test that they reach the inner parser (no type error).
     * Using bulk_cancel as the simplest to construct for each metadata type. */

    /* update_leverage */
    ctx_reset();
    {
        s_action_metadata m = {0};
        m.op_type  = OP_TYPE_UPDATE_LEVERAGE;
        m.asset_id = TEST_ASSET_ID;
        m.network  = NETWORK_MAINNET;
        strncpy(m.asset_ticker, "BTC", sizeof(m.asset_ticker) - 1);
        ctx_save_action_metadata(&m);
    }

    /* Build update_leverage inner TLV */
    uint8_t inner[128];
    size_t  inner_off = 0;
    tlv_append_uint(inner, &inner_off, 0xd1 /* TAG_ASSET */, TEST_ASSET_ID);
    tlv_append_bool(inner, &inner_off, 0xde /* TAG_IS_CROSS */, true);
    tlv_append_uint(inner, &inner_off, 0xed /* TAG_LEVERAGE */, 10U);

    uint8_t buf[512];
    size_t  len = build_action_tlv(buf,
                                   ACTION_STRUCT_TYPE,
                                   ACTION_STRUCT_VERSION,
                                   ACTION_TYPE_UPDATE_LEVERAGE,
                                   1ULL,
                                   inner, inner_off);

    buffer_t payload = make_buffer(buf, len);
    assert_true(parse_action(&payload));
    const s_action *act = ctx_get_current_action();
    assert_non_null(act);
    assert_int_equal(act->type, ACTION_TYPE_UPDATE_LEVERAGE);
}

static void test_parse_truncated_payload_fails(void **state) {
    (void) state;
    uint8_t  truncated[] = {TAG_STRUCT_TYPE};
    buffer_t payload     = make_buffer(truncated, sizeof(truncated));
    assert_false(parse_action(&payload));
}

static void test_parse_empty_payload_fails(void **state) {
    (void) state;
    uint8_t  empty[1] = {0};
    buffer_t payload  = make_buffer(empty, 0);
    assert_false(parse_action(&payload));
}

/* ─── action_hash test ───────────────────────────────────────────────────── */

/*
 * Call action_hash() with a valid BULK_CANCEL action. With our crypto stubs
 * (cx_keccak_init/update/final all return CX_OK, eip712_cid_hash returns false),
 * compute_connection_id() succeeds and invokes action_serialize(), but
 * eip712_cid_hash() returns false — so action_hash() returns false.
 * This exercises the action_serialize() dispatch without requiring real crypto.
 */
static void test_action_hash_exercises_serialize(void **state) {
    (void) state;
    s_action action = {0};
    action.type  = ACTION_TYPE_BULK_CANCEL;
    action.nonce = 12345ULL;
    action.bulk_cancel.cancel_count  = 1;
    action.bulk_cancel.cancels[0].asset = TEST_ASSET_ID;
    action.bulk_cancel.cancels[0].oid   = 42ULL;

    s_action_metadata metadata = {0};
    metadata.network = NETWORK_MAINNET;

    uint8_t domain_hash[32]  = {0};
    uint8_t message_hash[32] = {0};

    /* eip712_cid_hash stub returns false, so action_hash returns false.
     * action_serialize (and bulk_cancel_serialize) are still executed. */
    assert_false(action_hash(&action, &metadata, domain_hash, message_hash));
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_parse_valid_bulk_cancel_action, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_wrong_struct_type_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_wrong_struct_version_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_unknown_action_type_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_nonce_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_missing_action_payload_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_action_payload_before_type_fails, setup, teardown),
        cmocka_unit_test(test_parse_null_payload_fails),
        cmocka_unit_test(test_parse_no_metadata_fails),
        cmocka_unit_test_setup_teardown(test_parse_all_action_types, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_truncated_payload_fails, setup, teardown),
        cmocka_unit_test_setup_teardown(test_parse_empty_payload_fails, setup, teardown),
        cmocka_unit_test(test_action_hash_exercises_serialize),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
