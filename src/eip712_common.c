#include <string.h>
#include "os_print.h"
#include "cx.h"
#include "write.h"
#include "eip712_common.h"
#include "crypto_helpers.h"

static const uint8_t EIP_712_MAGIC[] = {0x19, 0x01};

bool eip712_compute_domain_hash(const char *name,
                                const char *version,
                                const uint64_t *chain_id,
                                const uint8_t *verifying_contract,
                                uint8_t *out) {
    cx_sha3_t hash_ctx;
    const char encoded_type[] =
        "EIP712Domain(string name,string version,uint256 chainId,address verifyingContract)";
    uint8_t tmp[sizeof(*chain_id)];

    if (cx_keccak_init_no_throw(&hash_ctx, 256) != CX_OK) {
        return false;
    }

    if (!eip712_encode_dyn_val(&hash_ctx, (uint8_t *) encoded_type, strlen(encoded_type))) {
        return false;
    }

    if (!eip712_encode_dyn_val(&hash_ctx, (uint8_t *) name, strlen(name))) {
        return false;
    }

    if (!eip712_encode_dyn_val(&hash_ctx, (uint8_t *) version, strlen(version))) {
        return false;
    }

    write_u64_be(tmp, 0, *chain_id);
    if (!eip712_encode_val(&hash_ctx, tmp, sizeof(tmp))) {
        return false;
    }

    if (!eip712_encode_val(&hash_ctx, verifying_contract, 20)) {
        return false;
    }

    if (cx_hash_final((cx_hash_t *) &hash_ctx, out) != CX_OK) {
        return false;
    }
    return true;
}

static bool eip712_hash_then_feed(cx_sha3_t *hash_ctx, const uint8_t *data, size_t length) {
    uint8_t tmp_hash[32];

    if (cx_keccak_256_hash(data, length, tmp_hash) != CX_OK) {
        return false;
    }
    return cx_hash_update((cx_hash_t *) hash_ctx, tmp_hash, sizeof(tmp_hash)) == CX_OK;
}

bool eip712_encode_dyn_val(cx_sha3_t *hash_ctx, const uint8_t *data, size_t length) {
    return eip712_hash_then_feed(hash_ctx, data, length);
}

bool eip712_encode_val(cx_sha3_t *hash_ctx, const uint8_t *data, size_t length) {
    uint8_t tmp[32];
    size_t offset;

    if (length > sizeof(tmp)) {
        return false;
    }
    offset = sizeof(tmp) - length;
    explicit_bzero(tmp, offset);
    memcpy(&tmp[offset], data, length);
    return cx_hash_update((cx_hash_t *) hash_ctx, tmp, sizeof(tmp)) == CX_OK;
}

bool eip712_sign(const uint32_t *bip32_path, size_t path_length, s_eip712_ctx *ctx) {
    cx_sha3_t hash_ctx;
    uint8_t hash[32];
    uint32_t info = 0;

    if (cx_keccak_init_no_throw(&hash_ctx, 256) != CX_OK) {
        return false;
    }

    if (cx_hash_update((cx_hash_t *) &hash_ctx, EIP_712_MAGIC, sizeof(EIP_712_MAGIC)) != CX_OK) {
        return false;
    }
    if (cx_hash_update((cx_hash_t *) &hash_ctx, ctx->domain_hash, sizeof(ctx->domain_hash)) !=
        CX_OK) {
        return false;
    }
    if (cx_hash_update((cx_hash_t *) &hash_ctx, ctx->message_hash, sizeof(ctx->message_hash)) !=
        CX_OK) {
        return false;
    }

    if (cx_hash_final((cx_hash_t *) &hash_ctx, hash) != CX_OK) {
        return false;
    }
    PRINTF("hash to sign = 0x%.*h\n", sizeof(hash), hash);
    if (bip32_derive_ecdsa_sign_rs_hash_256(CX_CURVE_256K1,
                                            bip32_path,
                                            path_length,
                                            CX_RND_RFC6979 | CX_LAST,
                                            CX_SHA256,
                                            hash,
                                            sizeof(hash),
                                            ctx->signature.r,
                                            ctx->signature.s,
                                            &info) != CX_OK) {
        return false;
    }
    ctx->signature.v = 27;
    if (info & CX_ECCINFO_PARITY_ODD) {
        ctx->signature.v += 1;
    }
    if (info & CX_ECCINFO_xGTn) {
        ctx->signature.v += 2;
    }
    return true;
}
