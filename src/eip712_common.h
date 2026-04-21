#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "cx.h"

typedef struct {
    uint8_t domain_hash[32];
    uint8_t message_hash[32];
    struct {
        uint8_t v;
        uint8_t r[32];
        uint8_t s[32];
    } signature;
} s_eip712_ctx;

bool eip712_encode_dyn_val(cx_sha3_t *hash_ctx, const uint8_t *data, size_t length);
bool eip712_encode_val(cx_sha3_t *hash_ctx, const uint8_t *data, size_t length);
bool eip712_compute_domain_hash(const char *name,
                                const char *version,
                                const uint64_t *chain_id,
                                const uint8_t *verifying_contract,
                                uint8_t *out);
bool eip712_sign(const uint32_t *bip32_path, size_t path_length, s_eip712_ctx *ctx);
