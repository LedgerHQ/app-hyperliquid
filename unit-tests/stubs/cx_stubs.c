/* Stub implementations of hardware crypto functions.
 * parse_action() never calls these; they exist only to satisfy the linker
 * when building the action shared library for unit tests.
 */
#include "cx.h"

cx_err_t cx_keccak_init_no_throw(cx_sha3_t *hash, size_t size) {
    (void) hash; (void) size;
    return CX_OK;
}

cx_err_t cx_hash_update(cx_hash_t *hash, const uint8_t *input, size_t in_len) {
    (void) hash; (void) input; (void) in_len;
    return CX_OK;
}

cx_err_t cx_hash_final(cx_hash_t *hash, uint8_t *digest) {
    (void) hash; (void) digest;
    return CX_OK;
}
