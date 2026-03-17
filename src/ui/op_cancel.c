#include <stdbool.h>
#include <stdio.h>
#include "ui_context.h"
#include "ui_common.h"
#include "hl_context.h"

bool ui_cancel(s_ui_ctx *ui_ctx, const s_action_metadata *metadata) {
    if (ctx_get_action(ACTION_TYPE_BULK_CANCEL) == NULL) {
        return false;
    }
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Operation";
    snprintf(ui_ctx->cancel.operation,
             sizeof(ui_ctx->cancel.operation),
             "Cancel order - %s",
             metadata->asset_ticker);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->cancel.operation;
    ui_ctx->pair_list.nbPairs += 1;

    show_metadata_margin(ui_ctx, metadata, ui_ctx->cancel.margin, sizeof(ui_ctx->cancel.margin));
    show_metadata_leverage(ui_ctx,
                           metadata,
                           ui_ctx->cancel.leverage,
                           sizeof(ui_ctx->cancel.leverage));

    snprintf(ui_ctx->intent,
             sizeof(ui_ctx->intent),
#ifdef SCREEN_SIZE_WALLET
             "cancel %s order",
             metadata->asset_ticker
#else
             "cancel order"
#endif
    );
    return true;
}
