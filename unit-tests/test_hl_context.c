#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "hl_context.h"

/* ─── helpers ────────────────────────────────────────────────────────────── */

static s_action_metadata make_metadata(uint32_t asset_id) {
    s_action_metadata m = {0};
    m.op_type  = OP_TYPE_ORDER;
    m.asset_id = asset_id;
    m.network  = NETWORK_MAINNET;
    strncpy(m.asset_ticker, "BTC", sizeof(m.asset_ticker) - 1);
    return m;
}

static s_action make_action(e_action_type type) {
    s_action a = {0};
    a.type  = type;
    a.nonce = 12345;
    return a;
}

/* ─── tests ──────────────────────────────────────────────────────────────── */

static void test_initial_state(void **state) {
    (void) state;
    ctx_reset();
    assert_null(ctx_get_action_metadata());
    assert_null(ctx_get_current_action());
    assert_true(ctx_current_action_is_first());
    assert_int_equal(ctx_remaining_actions(), 0);
}

static void test_save_and_get_metadata(void **state) {
    (void) state;
    ctx_reset();
    s_action_metadata m = make_metadata(42);

    ctx_save_action_metadata(&m);
    const s_action_metadata *got = ctx_get_action_metadata();

    assert_non_null(got);
    assert_int_equal(got->asset_id, 42);
    assert_int_equal(got->op_type, OP_TYPE_ORDER);
    assert_int_equal(got->network, NETWORK_MAINNET);
    assert_string_equal(got->asset_ticker, "BTC");
}

static void test_save_metadata_null_noop(void **state) {
    (void) state;
    ctx_reset();
    ctx_save_action_metadata(NULL);
    assert_null(ctx_get_action_metadata());
}

static void test_push_and_get_action(void **state) {
    (void) state;
    ctx_reset();

    s_action a = make_action(ACTION_TYPE_BULK_CANCEL);
    assert_true(ctx_push_action(&a));

    const s_action *got = ctx_get_current_action();
    assert_non_null(got);
    assert_int_equal(got->type, ACTION_TYPE_BULK_CANCEL);
    assert_int_equal(got->nonce, 12345);
}

static void test_push_duplicate_type_fails(void **state) {
    (void) state;
    ctx_reset();

    s_action a = make_action(ACTION_TYPE_BULK_ORDER);
    assert_true(ctx_push_action(&a));
    /* pushing the same type again must fail */
    assert_false(ctx_push_action(&a));
}

static void test_push_different_types(void **state) {
    (void) state;
    ctx_reset();

    s_action a1 = make_action(ACTION_TYPE_BULK_ORDER);
    s_action a2 = make_action(ACTION_TYPE_BULK_CANCEL);
    assert_true(ctx_push_action(&a1));
    assert_true(ctx_push_action(&a2));
    assert_int_equal(ctx_remaining_actions(), 2);
}

static void test_current_action_is_first(void **state) {
    (void) state;
    ctx_reset();

    s_action a = make_action(ACTION_TYPE_UPDATE_LEVERAGE);
    assert_true(ctx_current_action_is_first());
    assert_true(ctx_push_action(&a));
    assert_true(ctx_current_action_is_first());
}

static void test_switch_to_next_action(void **state) {
    (void) state;
    ctx_reset();

    s_action a1 = make_action(ACTION_TYPE_BULK_ORDER);
    s_action a2 = make_action(ACTION_TYPE_BULK_CANCEL);
    ctx_push_action(&a1);
    ctx_push_action(&a2);

    assert_true(ctx_current_action_is_first());
    assert_int_equal(ctx_remaining_actions(), 2);

    ctx_switch_to_next_action();

    assert_false(ctx_current_action_is_first());
    assert_int_equal(ctx_remaining_actions(), 1);

    const s_action *cur = ctx_get_current_action();
    assert_non_null(cur);
    assert_int_equal(cur->type, ACTION_TYPE_BULK_CANCEL);
}

static void test_remaining_actions_decreases(void **state) {
    (void) state;
    ctx_reset();

    s_action a1 = make_action(ACTION_TYPE_BULK_ORDER);
    s_action a2 = make_action(ACTION_TYPE_BULK_CANCEL);
    s_action a3 = make_action(ACTION_TYPE_UPDATE_LEVERAGE);
    ctx_push_action(&a1);
    ctx_push_action(&a2);
    ctx_push_action(&a3);

    assert_int_equal(ctx_remaining_actions(), 3);
    ctx_switch_to_next_action();
    assert_int_equal(ctx_remaining_actions(), 2);
    ctx_switch_to_next_action();
    assert_int_equal(ctx_remaining_actions(), 1);
    ctx_switch_to_next_action();
    assert_int_equal(ctx_remaining_actions(), 0);
    assert_null(ctx_get_current_action());
}

static void test_get_action_by_type(void **state) {
    (void) state;
    ctx_reset();

    s_action a1 = make_action(ACTION_TYPE_BULK_ORDER);
    s_action a2 = make_action(ACTION_TYPE_APPROVE_BUILDER_FEE);
    ctx_push_action(&a1);
    ctx_push_action(&a2);

    const s_action *found = ctx_get_action(ACTION_TYPE_APPROVE_BUILDER_FEE);
    assert_non_null(found);
    assert_int_equal(found->type, ACTION_TYPE_APPROVE_BUILDER_FEE);

    assert_null(ctx_get_action(ACTION_TYPE_BULK_CANCEL));
}

static void test_reset_clears_state(void **state) {
    (void) state;

    s_action_metadata m = make_metadata(7);
    s_action          a = make_action(ACTION_TYPE_BULK_MODIFY);
    ctx_save_action_metadata(&m);
    ctx_push_action(&a);

    ctx_reset();

    assert_null(ctx_get_action_metadata());
    assert_null(ctx_get_current_action());
    assert_int_equal(ctx_remaining_actions(), 0);
    assert_true(ctx_current_action_is_first());
}

static void test_push_beyond_max_fails(void **state) {
    (void) state;
    ctx_reset();

    /* Push all 6 distinct action types (all that exist); verify the 7th attempt
       returns false due to duplicate rejection.                                */
    static const e_action_type types[] = {
        ACTION_TYPE_BULK_ORDER,
        ACTION_TYPE_BULK_MODIFY,
        ACTION_TYPE_BULK_CANCEL,
        ACTION_TYPE_UPDATE_LEVERAGE,
        ACTION_TYPE_APPROVE_BUILDER_FEE,
        ACTION_TYPE_UPDATE_ISOLATED_MARGIN,
    };

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
        s_action a = make_action(types[i]);
        assert_true(ctx_push_action(&a));
    }
    assert_int_equal(ctx_remaining_actions(), 6);

    /* A duplicate must be rejected */
    s_action extra = make_action(ACTION_TYPE_BULK_ORDER);
    assert_false(ctx_push_action(&extra));
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_initial_state),
        cmocka_unit_test(test_save_and_get_metadata),
        cmocka_unit_test(test_save_metadata_null_noop),
        cmocka_unit_test(test_push_and_get_action),
        cmocka_unit_test(test_push_duplicate_type_fails),
        cmocka_unit_test(test_push_different_types),
        cmocka_unit_test(test_current_action_is_first),
        cmocka_unit_test(test_switch_to_next_action),
        cmocka_unit_test(test_remaining_actions_decreases),
        cmocka_unit_test(test_get_action_by_type),
        cmocka_unit_test(test_reset_clears_state),
        cmocka_unit_test(test_push_beyond_max_fails),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
