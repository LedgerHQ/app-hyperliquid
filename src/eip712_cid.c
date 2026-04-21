#include <string.h>
#include "os_print.h"
#include "cx.h"
#include "write.h"
#include "eip712_cid.h"
#include "eip712_common.h"
#include "constants.h"

static bool compute_message_hash(const char *source, const uint8_t *cid, uint8_t *out) {
    cx_sha3_t hash_ctx;
    const char encoded_type[] = "Agent(string source,bytes32 connectionId)";

    if (cx_keccak_init_no_throw(&hash_ctx, 256) != CX_OK) {
        return false;
    }

    if (!eip712_encode_dyn_val(&hash_ctx, (uint8_t *) encoded_type, strlen(encoded_type))) {
        return false;
    }

    if (!eip712_encode_dyn_val(&hash_ctx, (uint8_t *) source, strlen(source))) {
        return false;
    }

    if (!eip712_encode_val(&hash_ctx, cid, 32)) {
        return false;
    }

    if (cx_hash_final((cx_hash_t *) &hash_ctx, out) != CX_OK) {
        return false;
    }
    return true;
}

bool eip712_cid_hash(const char *source,
                     const uint8_t *cid,
                     uint8_t *domain_hash,
                     uint8_t *message_hash) {
    PRINTF("======== computing EIP-712 hashes ========\n");
    PRINTF("source = \"%s\"\n", source);
    PRINTF("cid = 0x%.*h\n", 32, cid);
    PRINTF("==========================================\n");

    // domain configuration
    const char name[] = "Exchange";
    const char version[] = "1";
    uint64_t chain_id = 1337;
    const uint8_t verifying_contract[ADDRESS_LENGTH] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    if (!eip712_compute_domain_hash(name, version, &chain_id, verifying_contract, domain_hash)) {
        return false;
    }
    if (!compute_message_hash(source, cid, message_hash)) {
        return false;
    }
    return true;
}
