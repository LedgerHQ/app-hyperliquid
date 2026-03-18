#pragma once
/* Stub: replaces the hardware crypto library header for unit tests.
 * Only the types and constants actually used by action.c are defined here.
 * All crypto functions are provided as no-op stubs in stubs/cx_stubs.c.
 */

#include <stdint.h>
#include <stddef.h>

typedef uint32_t cx_err_t;
#define CX_OK 0u

/* Opaque hash context types — large enough for stack allocation in action.c */
typedef struct { uint8_t _opaque[256]; } cx_sha3_t;
typedef struct { uint8_t _opaque[256]; } cx_hash_t;

cx_err_t cx_keccak_init_no_throw(cx_sha3_t *hash, size_t size);
cx_err_t cx_hash_update(cx_hash_t *hash, const uint8_t *input, size_t in_len);
cx_err_t cx_hash_final(cx_hash_t *hash, uint8_t *digest);
