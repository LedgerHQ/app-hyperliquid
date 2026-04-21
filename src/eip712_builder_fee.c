#include <string.h>
#include "os_print.h"
#include "cx.h"
#include "write.h"
#include "format.h"
#include "eip712_builder_fee.h"
#include "eip712_common.h"
#include "constants.h"

static bool compute_message_hash(const char *chain,
                                 const char *max_fee_rate,
                                 const uint8_t *builder,
                                 const uint64_t *nonce,
                                 uint8_t *out) {
    cx_sha3_t hash_ctx;
    // clang-format off
    const char encoded_type[] = "HyperliquidTransaction:ApproveBuilderFee(string hyperliquidChain,string maxFeeRate,address builder,uint64 nonce)";
    // clang-format on
    uint8_t tmp[sizeof(*nonce)];

    if (cx_keccak_init_no_throw(&hash_ctx, 256) != CX_OK) {
        return false;
    }

    if (!eip712_encode_dyn_val(&hash_ctx, (uint8_t *) encoded_type, strlen(encoded_type))) {
        return false;
    }

    if (!eip712_encode_dyn_val(&hash_ctx, (uint8_t *) chain, strlen(chain))) {
        return false;
    }

    if (!eip712_encode_dyn_val(&hash_ctx, (uint8_t *) max_fee_rate, strlen(max_fee_rate))) {
        return false;
    }

    if (!eip712_encode_val(&hash_ctx, builder, 20)) {
        return false;
    }

    write_u64_be(tmp, 0, *nonce);
    if (!eip712_encode_val(&hash_ctx, tmp, sizeof(tmp))) {
        return false;
    }

    if (cx_hash_final((cx_hash_t *) &hash_ctx, out) != CX_OK) {
        return false;
    }
    return true;
}

bool eip712_builder_fee_hash(const uint64_t *chain_id,
                             const char *chain,
                             const char *max_fee_rate,
                             const uint8_t *builder,
                             const uint64_t *nonce,
                             uint8_t *domain_hash,
                             uint8_t *message_hash) {
    char tmp[20 + 1];
    PRINTF("======== computing EIP-712 hashes ========\n");
    format_u64(tmp, sizeof(tmp), *chain_id);
    PRINTF("chain_id = %s\n", tmp);
    PRINTF("chain = \"%s\"\n", chain);
    PRINTF("max_fee_rate = \"%s\"\n", max_fee_rate);
    PRINTF("builder = 0x%.*h\n", 20, builder);
    format_u64(tmp, sizeof(tmp), *nonce);
    PRINTF("nonce = %s\n", tmp);
    PRINTF("==========================================\n");

    // domain configuration
    const char name[] = "HyperliquidSignTransaction";
    const char version[] = "1";
    const uint8_t verifying_contract[ADDRESS_LENGTH] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    if (!eip712_compute_domain_hash(name, version, chain_id, verifying_contract, domain_hash)) {
        return false;
    }
    if (!compute_message_hash(chain, max_fee_rate, builder, nonce, message_hash)) {
        return false;
    }
    return true;
}
