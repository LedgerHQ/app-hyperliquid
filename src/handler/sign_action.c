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
    int ret;
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
    if ((ret = io_send_response_pointer(buf, sizeof(buf), SWO_SUCCESS)) >= 0) {
        // successful response
        if (buf[0] == 0) {
            // was the last signature
            ctx_reset();
        }
    }
    return ret;
}

int handler_sign_action(const buffer_t *payload) {
    s_signing_ctx tmp = {0};

    if (payload->size < sizeof(tmp.bip32_path_length)) {
        return io_send_sw(SWO_WRONG_DATA_LENGTH);
    }
    tmp.bip32_path_length = payload->ptr[0];
    if (!bip32_path_read(&payload->ptr[sizeof(tmp.bip32_path_length)],
                         payload->size - sizeof(tmp.bip32_path_length),
                         tmp.bip32_path,
                         tmp.bip32_path_length)) {
        return io_send_sw(SWO_INCORRECT_DATA);
    }

    PRINTF("handler_sign_action(\"");
    for (int i = 0; i < (int) tmp.bip32_path_length; ++i) {
        if (i > 0) {
            PRINTF("/");
        }
        PRINTF("0x%08x", tmp.bip32_path[i]);
    }
    PRINTF("\")\n");

    // The UI review screen is shown only for the first SIGN_ACTION in a
    // session. Subsequent SIGN_ACTION commands (e.g. for TP/SL orders or an
    // accompanying APPROVE_BUILDER_FEE) are signed silently, by design:
    // all actions in the batch are described to the user in a single review
    // screen before any signature is produced, so repeating the review for
    // every intermediate action would be redundant and confusing.
    // The action list is frozen at that point (ctx_push_action rejects new
    // entries once action_index > 0), and each action type is validated
    // against the declared op_type at SET_ACTION time, so the set of actions
    // signed is guaranteed to match what was shown on screen.
    if (ctx_current_action_is_first()) {
        memcpy(&g_signing_ctx, &tmp, sizeof(g_signing_ctx));

        if (ctx_remaining_actions() == 0) {
            // nothing to sign
            return io_send_sw(SWO_INCORRECT_DATA);
        }
        if (!handle_ui(ctx_get_action_metadata())) {
            return io_send_sw(SWO_INCORRECT_DATA);
        }
        return 0;
    }

    if (memcmp(&tmp, &g_signing_ctx, sizeof(g_signing_ctx)) != 0) {
        PRINTF("Error: derivation path does not match!\n");
        return io_send_sw(SWO_INCORRECT_DATA);
    }
    return sign_action();
}
