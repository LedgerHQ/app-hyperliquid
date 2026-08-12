#include "cx.h"
#include "crypto_helpers.h"
#include "derive_addr_from_path.h"
#include "constants.h"

bool derive_addr_from_path(const uint32_t *bip32_path,
                           uint8_t path_length,
                           uint8_t out[ADDRESS_LENGTH]) {
    uint8_t raw_pk[65];
    uint8_t hashed_pk[32];

    if (bip32_path == NULL) {
        return false;
    }
    if (bip32_derive_get_pubkey_256(CX_CURVE_256K1,
                                    bip32_path,
                                    path_length,
                                    raw_pk,
                                    NULL,
                                    CX_SHA512) != CX_OK) {
        return false;
    }
    if (cx_keccak_256_hash(&raw_pk[1], sizeof(raw_pk) - 1, hashed_pk) != CX_OK) {
        return false;
    }
    memcpy(out, hashed_pk + sizeof(hashed_pk) - ADDRESS_LENGTH, ADDRESS_LENGTH);
    return true;
}
