#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "buffer.h"
#include "chunked_payload.h"

/*
 * Mirror of the internal buffer size defined in chunked_payload.c.
 * Keep in sync if that constant changes.
 */
#define PAYLOAD_BUFFER_SIZE (0xff * 4)

/* ─── mock handlers ──────────────────────────────────────────────────────── */

static bool             g_handler_called;
static bool             g_handler_return_value;
static uint8_t          g_handler_received[PAYLOAD_BUFFER_SIZE];
static size_t           g_handler_received_size;

static bool mock_handler(const buffer_t *buf) {
    g_handler_called = true;
    if (buf && buf->size <= sizeof(g_handler_received)) {
        memcpy(g_handler_received, buf->ptr, buf->size);
        g_handler_received_size = buf->size;
    }
    return g_handler_return_value;
}

static bool mock_handler_alt(const buffer_t *buf) {
    (void) buf;
    return true;
}

/* ─── helpers ────────────────────────────────────────────────────────────── */

/**
 * Writes a first-chunk APDU into @out: 2-byte BE total-size prefix followed by
 * @data_len bytes from @data.  Returns the total number of bytes written.
 */
static size_t make_first_chunk(uint8_t       *out,
                                const uint8_t *data,
                                size_t         data_len,
                                uint16_t       total_len) {
    out[0] = (uint8_t) (total_len >> 8);
    out[1] = (uint8_t) (total_len & 0xFF);
    if (data && data_len) {
        memcpy(out + 2, data, data_len);
    }
    return 2 + data_len;
}

/* ─── setup ──────────────────────────────────────────────────────────────── */

static int setup(void **state) {
    (void) state;
    g_handler_called        = false;
    g_handler_return_value  = true;
    g_handler_received_size = 0;
    memset(g_handler_received, 0, sizeof(g_handler_received));
    return 0;
}

/* ─── tests ──────────────────────────────────────────────────────────────── */

static void test_null_data_returns_false(void **state) {
    (void) state;
    assert_false(process_chunked_payload(true, NULL, mock_handler));
    assert_false(g_handler_called);
}

static void test_first_chunk_too_small_for_size_field_fails(void **state) {
    (void) state;
    /* One byte is not enough to read the 2-byte declared size */
    uint8_t one_byte = 0x42;
    buffer_t buf = {.ptr = &one_byte, .size = 1, .offset = 0};
    assert_false(process_chunked_payload(true, &buf, mock_handler));
    assert_false(g_handler_called);
}

static void test_single_chunk_happy_path(void **state) {
    (void) state;
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t apdu[2 + sizeof(payload)];
    size_t  apdu_len = make_first_chunk(apdu, payload, sizeof(payload), sizeof(payload));

    buffer_t buf = {.ptr = apdu, .size = apdu_len, .offset = 0};
    assert_true(process_chunked_payload(true, &buf, mock_handler));
    assert_true(g_handler_called);
    assert_int_equal(g_handler_received_size, sizeof(payload));
    assert_memory_equal(g_handler_received, payload, sizeof(payload));
}

static void test_two_chunks_happy_path(void **state) {
    (void) state;
    uint8_t payload[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    const size_t half = 3;

    /* First chunk: header + first half */
    uint8_t apdu1[2 + half];
    size_t  len1 = make_first_chunk(apdu1, payload, half, sizeof(payload));
    buffer_t buf1 = {.ptr = apdu1, .size = len1, .offset = 0};
    assert_true(process_chunked_payload(true, &buf1, mock_handler));
    assert_false(g_handler_called);  /* not complete yet */

    /* Second chunk: remaining half */
    buffer_t buf2 = {.ptr = payload + half, .size = sizeof(payload) - half, .offset = 0};
    assert_true(process_chunked_payload(false, &buf2, mock_handler));
    assert_true(g_handler_called);
    assert_int_equal(g_handler_received_size, sizeof(payload));
    assert_memory_equal(g_handler_received, payload, sizeof(payload));
}

static void test_many_chunks_happy_path(void **state) {
    (void) state;
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    const size_t chunk = 3;

    uint8_t apdu1[2 + chunk];
    size_t  len1 = make_first_chunk(apdu1, payload, chunk, sizeof(payload));
    buffer_t buf1 = {.ptr = apdu1, .size = len1, .offset = 0};
    assert_true(process_chunked_payload(true, &buf1, mock_handler));
    assert_false(g_handler_called);

    buffer_t buf2 = {.ptr = payload + chunk, .size = chunk, .offset = 0};
    assert_true(process_chunked_payload(false, &buf2, mock_handler));
    assert_false(g_handler_called);

    buffer_t buf3 = {.ptr = payload + 2 * chunk, .size = chunk, .offset = 0};
    assert_true(process_chunked_payload(false, &buf3, mock_handler));
    assert_true(g_handler_called);
    assert_int_equal(g_handler_received_size, sizeof(payload));
    assert_memory_equal(g_handler_received, payload, sizeof(payload));
}

static void test_oversized_declared_size_fails(void **state) {
    (void) state;
    /* Declared size one byte beyond the buffer capacity — must be rejected */
    uint16_t oversized = PAYLOAD_BUFFER_SIZE + 1;
    uint8_t  apdu[2]   = {(uint8_t)(oversized >> 8), (uint8_t)(oversized & 0xFF)};
    buffer_t buf = {.ptr = apdu, .size = sizeof(apdu), .offset = 0};
    assert_false(process_chunked_payload(true, &buf, mock_handler));
    assert_false(g_handler_called);
}

static void test_exact_max_size_accepted(void **state) {
    (void) state;
    /*
     * Declaring exactly PAYLOAD_BUFFER_SIZE must pass the bounds check.
     * Only the header is sent here; the handler must not be called yet.
     * Abort the dangling session at the end so subsequent tests start clean.
     */
    uint16_t max = PAYLOAD_BUFFER_SIZE;
    uint8_t  apdu[2] = {(uint8_t)(max >> 8), (uint8_t)(max & 0xFF)};
    buffer_t buf = {.ptr = apdu, .size = sizeof(apdu), .offset = 0};
    assert_true(process_chunked_payload(true, &buf, mock_handler));
    assert_false(g_handler_called);

    /* A second P1_FIRST hits the session-in-progress guard → explicit_bzero. */
    assert_false(process_chunked_payload(true, &buf, mock_handler));
}

static void test_chunk_exceeds_declared_size_fails(void **state) {
    (void) state;
    /* Declare 3 bytes but embed 5 bytes of data in the first chunk */
    uint8_t payload[]      = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t apdu[2 + sizeof(payload)];
    size_t  apdu_len = make_first_chunk(apdu, payload, sizeof(payload), 3 /* declared */);
    buffer_t buf = {.ptr = apdu, .size = apdu_len, .offset = 0};
    assert_false(process_chunked_payload(true, &buf, mock_handler));
    assert_false(g_handler_called);
}

static void test_following_without_first_fails(void **state) {
    (void) state;
    /*
     * After setup the internal handler field is NULL.  Sending P1_FOLLOWING
     * with any non-NULL handler must be rejected by the mismatch check.
     */
    uint8_t  data[] = {0x01, 0x02};
    buffer_t buf = {.ptr = data, .size = sizeof(data), .offset = 0};
    assert_false(process_chunked_payload(false, &buf, mock_handler));
    assert_false(g_handler_called);
}

static void test_cross_command_interleave_fails(void **state) {
    (void) state;
    /* Begin a session for mock_handler */
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t apdu1[2 + 2];
    size_t  len1 = make_first_chunk(apdu1, payload, 2, sizeof(payload));
    buffer_t buf1 = {.ptr = apdu1, .size = len1, .offset = 0};
    assert_true(process_chunked_payload(true, &buf1, mock_handler));
    assert_false(g_handler_called);

    /* Continuation for a different handler — must be rejected */
    buffer_t buf2 = {.ptr = payload + 2, .size = 2, .offset = 0};
    assert_false(process_chunked_payload(false, &buf2, mock_handler_alt));
    assert_false(g_handler_called);
}

static void test_empty_following_chunk_fails(void **state) {
    (void) state;
    /* Start a valid 4-byte session */
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t apdu1[2 + 2];
    size_t  len1 = make_first_chunk(apdu1, payload, 2, sizeof(payload));
    buffer_t buf1 = {.ptr = apdu1, .size = len1, .offset = 0};
    assert_true(process_chunked_payload(true, &buf1, mock_handler));
    assert_false(g_handler_called);

    /* Empty following chunk must be rejected */
    uint8_t dummy = 0;
    buffer_t buf2 = {.ptr = &dummy, .size = 0, .offset = 0};
    assert_false(process_chunked_payload(false, &buf2, mock_handler));
    assert_false(g_handler_called);
}

static void test_handler_failure_propagated(void **state) {
    (void) state;
    g_handler_return_value = false;

    uint8_t payload[] = {0xDE, 0xAD};
    uint8_t apdu[2 + sizeof(payload)];
    size_t  apdu_len = make_first_chunk(apdu, payload, sizeof(payload), sizeof(payload));
    buffer_t buf = {.ptr = apdu, .size = apdu_len, .offset = 0};
    assert_false(process_chunked_payload(true, &buf, mock_handler));
    assert_true(g_handler_called);  /* handler was invoked but returned false */
}

/* ─── main ───────────────────────────────────────────────────────────────── */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup(test_null_data_returns_false,                    setup),
        cmocka_unit_test_setup(test_first_chunk_too_small_for_size_field_fails, setup),
        cmocka_unit_test_setup(test_single_chunk_happy_path,                    setup),
        cmocka_unit_test_setup(test_two_chunks_happy_path,                      setup),
        cmocka_unit_test_setup(test_many_chunks_happy_path,                     setup),
        cmocka_unit_test_setup(test_oversized_declared_size_fails,              setup),
        cmocka_unit_test_setup(test_exact_max_size_accepted,                    setup),
        cmocka_unit_test_setup(test_chunk_exceeds_declared_size_fails,          setup),
        cmocka_unit_test_setup(test_following_without_first_fails,              setup),
        cmocka_unit_test_setup(test_cross_command_interleave_fails,             setup),
        cmocka_unit_test_setup(test_empty_following_chunk_fails,                setup),
        cmocka_unit_test_setup(test_handler_failure_propagated,                 setup),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
