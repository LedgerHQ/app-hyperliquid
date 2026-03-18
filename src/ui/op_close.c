#include <stdbool.h>
#include <stdio.h>
#include "action.h"
#include "hl_context.h"
#include "ui_context.h"
#include "ui_common.h"

bool ui_close(s_ui_ctx *ui_ctx, const s_action_metadata *metadata) {
    const s_action *action;
    const s_order_request *limit;

    if ((action = ctx_get_action(ACTION_TYPE_BULK_ORDER)) == NULL) {
        return false;
    }
    if ((limit = get_order_request(action->bulk_order.orders,
                                   action->bulk_order.order_count,
                                   &get_limit_request)) == NULL) {
        return false;
    }

    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Position to close";
    snprintf(ui_ctx->close.operation,
             sizeof(ui_ctx->close.operation),
             "%s - %s",
             get_short_long_string_capitalized(limit),
             metadata->asset_ticker);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->close.operation;
    ui_ctx->pair_list.nbPairs += 1;

    show_metadata_margin(ui_ctx, metadata, ui_ctx->close.margin, sizeof(ui_ctx->close.margin));
    show_metadata_leverage(ui_ctx,
                           metadata,
                           ui_ctx->close.leverage,
                           sizeof(ui_ctx->close.leverage));

    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Size";
    snprintf(ui_ctx->close.size,
             sizeof(ui_ctx->close.size),
             "%s %s",
             limit->sz,
             metadata->asset_ticker);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->close.size;
    ui_ctx->pair_list.nbPairs += 1;

    snprintf(ui_ctx->intent,
             sizeof(ui_ctx->intent),
#ifdef SCREEN_SIZE_WALLET
             "close %s %s position",
             metadata->asset_ticker,
             get_short_long_string(limit)
#else
             "close %s position",
             get_short_long_string(limit)
#endif
    );
    return true;
}
