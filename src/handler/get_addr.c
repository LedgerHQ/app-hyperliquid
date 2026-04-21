#include "io.h"
#include "status_words.h"
#include "bip32.h"
#include "cx.h"
#include "crypto_helpers.h"
#include "get_addr.h"
#include "constants.h"

int handler_get_addr(const buffer_t *payload) {
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t path_length;
    uint8_t raw_pk[65];
    uint8_t hashed_pk[32];

    if (payload->size < sizeof(path_length)) {
        return io_send_sw(SWO_WRONG_DATA_LENGTH);
    }
    path_length = payload->ptr[0];
    if (!bip32_path_read(&payload->ptr[sizeof(path_length)],
                         payload->size - sizeof(path_length),
                         bip32_path,
                         path_length)) {
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    if (bip32_derive_get_pubkey_256(CX_CURVE_256K1,
                                    bip32_path,
                                    path_length,
                                    raw_pk,
                                    NULL,
                                    CX_SHA512) != CX_OK) {
        return io_send_sw(SWO_PARAMETER_ERROR_NO_INFO);
    }
    if (cx_keccak_256_hash(&raw_pk[1], sizeof(raw_pk) - 1, hashed_pk) != CX_OK) {
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    return io_send_response_pointer(hashed_pk + sizeof(hashed_pk) - ADDRESS_LENGTH,
                                    ADDRESS_LENGTH,
                                    SWO_SUCCESS);
}
