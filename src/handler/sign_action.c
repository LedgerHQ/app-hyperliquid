#include <string.h>
#include "os_print.h"
#include "io.h"
#include "status_words.h"
#include "bip32.h"
#include "sign_action.h"
#include "hl_context.h"
#include "eip712_common.h"
#include "display.h"

typedef struct {
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t bip32_path_length;
} s_signing_ctx;

static s_signing_ctx g_signing_ctx;

int sign_action(void) {
    s_eip712_ctx eip712_ctx;
    uint8_t buf[1 + sizeof(eip712_ctx.signature)];
    const s_action *action;
    const s_action_metadata *metadata;

    if ((action = ctx_get_current_action()) == NULL) {
        PRINTF("Error: could not get action!\n");
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    if ((metadata = ctx_get_action_metadata()) == NULL) {
        PRINTF("Error: missing action metadata!\n");
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    if (!action_hash(action, metadata, eip712_ctx.domain_hash, eip712_ctx.message_hash)) {
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    PRINTF("domain hash = 0x%.*h\n", sizeof(eip712_ctx.domain_hash), eip712_ctx.domain_hash);
    PRINTF("message hash = 0x%.*h\n", sizeof(eip712_ctx.message_hash), eip712_ctx.message_hash);
    if (!eip712_sign(g_signing_ctx.bip32_path, g_signing_ctx.bip32_path_length, &eip712_ctx)) {
        PRINTF("Error: could not sign EIP-712 message!\n");
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    ctx_switch_to_next_action();

    buf[0] = ctx_remaining_actions();
    memcpy(&buf[1], &eip712_ctx.signature, sizeof(eip712_ctx.signature));
    return io_send_response_pointer(buf, sizeof(buf), SWO_SUCCESS);
}

int handler_sign_action(const buffer_t *payload) {
    if (payload->size < sizeof(g_signing_ctx.bip32_path_length)) {
        return io_send_sw(SWO_WRONG_DATA_LENGTH);
    }
    g_signing_ctx.bip32_path_length = payload->ptr[0];
    if (!bip32_path_read(&payload->ptr[sizeof(g_signing_ctx.bip32_path_length)],
                         payload->size - sizeof(g_signing_ctx.bip32_path_length),
                         g_signing_ctx.bip32_path,
                         g_signing_ctx.bip32_path_length)) {
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    PRINTF("handler_sign_action(\"");
    for (int i = 0; i < (int) g_signing_ctx.bip32_path_length; ++i) {
        if (i > 0) {
            PRINTF("/");
        }
        PRINTF("0x%08x", g_signing_ctx.bip32_path[i]);
    }
    PRINTF("\")\n");

    if (ctx_current_action_is_first()) {
        if (!handle_ui(ctx_get_action_metadata())) {
            return io_send_sw(SWO_INCORRECT_DATA);
        }
        return 0;
    }

    return sign_action();
}
