#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "os_print.h"
#include "io.h"
#include "status_words.h"
#include "nbgl_use_case.h"
#include "format.h"
#include "hl_context.h"
#include "menu.h"
#include "ui_context.h"
#include "sign_action.h"
#include "operations.h"

#ifdef SCREEN_SIZE_WALLET
#define APP_REVIEW_ICON LARGE_REVIEW_ICON
#else
#define APP_REVIEW_ICON REVIEW_ICON
#endif

static s_ui_ctx g_ui_ctx;

static void review_choice(bool confirm) {
    if (confirm) {
        if (sign_action() != -1) {
            nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_SIGNED, ui_menu_main);
        } else {
            ctx_reset();
            ui_menu_main();
        }
    } else {
        ctx_reset();
        io_send_sw(SWO_CONDITIONS_NOT_SATISFIED);
        ui_menu_main();
    }
}

static void prepare_review_sign_msgs(s_ui_ctx *ui_ctx) {
    snprintf(ui_ctx->review_msg,
             sizeof(ui_ctx->review_msg),
             "Review message to %s",
             ui_ctx->intent);
    snprintf(ui_ctx->sign_msg, sizeof(ui_ctx->sign_msg), "Sign message to %s?", ui_ctx->intent);
}

bool handle_ui(const s_action_metadata *metadata) {
    bool ret;

    if (metadata == NULL) {
        return false;
    }
    explicit_bzero(&g_ui_ctx, sizeof(g_ui_ctx));
    g_ui_ctx.pair_list.pairs = (nbgl_contentTagValue_t *) &g_ui_ctx.pairs;

    g_ui_ctx.pairs[g_ui_ctx.pair_list.nbPairs].item = "Protocol";
    g_ui_ctx.pairs[g_ui_ctx.pair_list.nbPairs].value = APPNAME;
    g_ui_ctx.pair_list.nbPairs += 1;

    switch (metadata->op_type) {
        case OP_TYPE_ORDER:
            ret = ui_order(&g_ui_ctx, metadata);
            break;
        case OP_TYPE_MODIFY:
            ret = ui_modify(&g_ui_ctx, metadata);
            break;
        case OP_TYPE_CANCEL:
            ret = ui_cancel(&g_ui_ctx, metadata);
            break;
        case OP_TYPE_CLOSE:
            ret = ui_close(&g_ui_ctx, metadata);
            break;
        default:
            PRINTF("Error: no UI flow for given operation type (%u)!\n", metadata->op_type);
            ret = false;
    }
    if (ret) {
        prepare_review_sign_msgs(&g_ui_ctx);
        nbgl_useCaseReview(TYPE_MESSAGE,
                           &g_ui_ctx.pair_list,
                           &APP_REVIEW_ICON,
                           g_ui_ctx.review_msg,
                           NULL,
                           g_ui_ctx.sign_msg,
                           &review_choice);
    }
    return ret;
}
