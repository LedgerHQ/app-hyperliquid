#include "io.h"
#include "status_words.h"
#include "bip32.h"
#include "get_addr.h"
#include "derive_addr_from_path.h"

int handler_get_addr(const buffer_t *payload) {
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t path_length;
    uint8_t addr[ADDRESS_LENGTH];

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
    if (!derive_addr_from_path(bip32_path, path_length, addr)) {
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    return io_send_response_pointer(addr, sizeof(addr), SWO_SUCCESS);
}
