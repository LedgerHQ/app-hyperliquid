#include <stdbool.h>
#include <stdio.h>
#include "os_print.h"
#include "format.h"
#include "ui_context.h"
#include "ui_common.h"
#include "hl_context.h"

static void show_cancel_oids(s_ui_ctx *ui_ctx, const s_bulk_cancel *bulk_cancel) {
    for (uint8_t i = 0; i < bulk_cancel->cancel_count; ++i) {
        format_u64(ui_ctx->cancel.oid[i],
                   sizeof(ui_ctx->cancel.oid[i]),
                   bulk_cancel->cancels[i].oid);
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Order ID";
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->cancel.oid[i];
        ui_ctx->pair_list.nbPairs += 1;
    }
}

bool ui_cancel(s_ui_ctx *ui_ctx, const s_action_metadata *metadata) {
    const s_action *action;

    if ((action = ctx_get_action(ACTION_TYPE_BULK_CANCEL)) == NULL) {
        return false;
    }
    if (action->bulk_cancel.cancel_count != 1) {
        PRINTF("Error: unexpected cancel count in bulk_cancel (%u)\n",
               action->bulk_cancel.cancel_count);
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

    show_cancel_oids(ui_ctx, &action->bulk_cancel);

    snprintf(ui_ctx->intent,
             sizeof(ui_ctx->intent),
#ifdef SCREEN_SIZE_WALLET
             "Cancel %s order",
             metadata->asset_ticker
#else
             "cancel order"
#endif
    );
    return true;
}

bool ui_cancel_tp_sl(s_ui_ctx *ui_ctx, const s_action_metadata *metadata) {
    const s_action *action;
    char target_tmp[32];

    if ((action = ctx_get_action(ACTION_TYPE_BULK_CANCEL)) == NULL) {
        return false;
    }

    // sanity check
    if (metadata->op_type == OP_TYPE_CANCEL_TP_SL) {
        if (action->bulk_cancel.cancel_count != 2) {
            return false;
        }
        snprintf(target_tmp,
                 sizeof(target_tmp),
#ifdef SCREEN_SIZE_WALLET
                 "Take Profit and Stop Loss"
#else
                 "TP/SL"
#endif
        );
    } else {
        if (action->bulk_cancel.cancel_count != 1) {
            return false;
        }
        if (metadata->op_type == OP_TYPE_CANCEL_TP) {
            snprintf(target_tmp,
                     sizeof(target_tmp),
#ifdef SCREEN_SIZE_WALLET
                     "Take Profit"
#else
                     "TP"
#endif
            );
        } else {
            snprintf(target_tmp,
                     sizeof(target_tmp),
#ifdef SCREEN_SIZE_WALLET
                     "Stop Loss"
#else
                     "SL"
#endif
            );
        }
    }

    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Operation";
    snprintf(ui_ctx->cancel.operation,
             sizeof(ui_ctx->cancel.operation),
             "Cancel order - %s %s",
             metadata->asset_ticker,
             target_tmp);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->cancel.operation;
    ui_ctx->pair_list.nbPairs += 1;

    show_metadata_margin(ui_ctx, metadata, ui_ctx->cancel.margin, sizeof(ui_ctx->cancel.margin));
    show_metadata_leverage(ui_ctx,
                           metadata,
                           ui_ctx->cancel.leverage,
                           sizeof(ui_ctx->cancel.leverage));

    show_cancel_oids(ui_ctx, &action->bulk_cancel);

    snprintf(ui_ctx->intent,
             sizeof(ui_ctx->intent),
#ifdef SCREEN_SIZE_WALLET
             "Cancel %s %s",
             metadata->asset_ticker,
             target_tmp
#else
             "cancel %s",
             target_tmp
#endif
    );
    return true;
}
