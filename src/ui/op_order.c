#include <stdio.h>
#include "ui_context.h"
#include "action.h"
#include "ui_common.h"
#include "hl_context.h"

static bool ui_order_common(s_ui_ctx *ui_ctx,
                            const s_action_metadata *metadata,
                            const s_order_request *limit,
                            const s_update_leverage *update_leverage,
                            const s_order_request *tp_order,
                            const s_order_request *sl_order) {
    show_metadata_margin(ui_ctx, metadata, ui_ctx->order.margin, sizeof(ui_ctx->order.margin));

    if (update_leverage != NULL) {
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Leverage";
        snprintf(ui_ctx->order.leverage,
                 sizeof(ui_ctx->order.leverage),
                 "%ux",
                 update_leverage->leverage);
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->order.leverage;
        ui_ctx->pair_list.nbPairs += 1;
    }

    if (tp_order != NULL) {
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Take Profit";
        snprintf(ui_ctx->order.tp_price,
                 sizeof(ui_ctx->order.tp_price),
                 "%s %s",
                 tp_order->trigger.trigger_px,
                 COUNTERVALUE_TICKER);
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->order.tp_price;
        ui_ctx->pair_list.nbPairs += 1;
    }

    if (sl_order != NULL) {
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Stop Loss";
        snprintf(ui_ctx->order.sl_price,
                 sizeof(ui_ctx->order.sl_price),
                 "%s %s",
                 sl_order->trigger.trigger_px,
                 COUNTERVALUE_TICKER);
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->order.sl_price;
        ui_ctx->pair_list.nbPairs += 1;
    }

    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Size";
    snprintf(ui_ctx->order.size,
             sizeof(ui_ctx->order.size),
             "%s %s",
             limit->sz,
             metadata->asset_ticker);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->order.size;
    ui_ctx->pair_list.nbPairs += 1;

    return true;
}

static bool ui_market_order(s_ui_ctx *ui_ctx,
                            const s_action_metadata *metadata,
                            const s_order_request *limit,
                            const s_update_leverage *update_leverage,
                            const s_order_request *tp_order,
                            const s_order_request *sl_order) {
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Operation";
    snprintf(ui_ctx->order.operation,
             sizeof(ui_ctx->order.operation),
             "Market %s - %s",
             get_short_long_string_capitalized(limit),
             metadata->asset_ticker);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->order.operation;
    ui_ctx->pair_list.nbPairs += 1;

    if (!ui_order_common(ui_ctx, metadata, limit, update_leverage, tp_order, sl_order)) {
        return false;
    }

    snprintf(ui_ctx->intent,
             sizeof(ui_ctx->intent),
#ifdef SCREEN_SIZE_WALLET
             "Open %s %s",
             metadata->asset_ticker,
             get_short_long_string(limit)
#else
             "open %s",
             get_short_long_string(limit)
#endif
    );
    return true;
}

static bool ui_limit_order(s_ui_ctx *ui_ctx,
                           const s_action_metadata *metadata,
                           const s_order_request *limit,
                           const s_update_leverage *update_leverage,
                           const s_order_request *tp_order,
                           const s_order_request *sl_order) {
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Operation";
    snprintf(ui_ctx->order.operation,
             sizeof(ui_ctx->order.operation),
             "Limit %s - %s",
             get_short_long_string_capitalized(limit),
             metadata->asset_ticker);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->order.operation;
    ui_ctx->pair_list.nbPairs += 1;

    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Limit Price";
    snprintf(ui_ctx->order.limit_price,
             sizeof(ui_ctx->order.limit_price),
             "%s %s",
             limit->limit_px,
             COUNTERVALUE_TICKER);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->order.limit_price;
    ui_ctx->pair_list.nbPairs += 1;

    if (!ui_order_common(ui_ctx, metadata, limit, update_leverage, tp_order, sl_order)) {
        return false;
    }

    snprintf(ui_ctx->intent,
             sizeof(ui_ctx->intent),
#ifdef SCREEN_SIZE_WALLET
             "Open %s limit order",
             metadata->asset_ticker
#else
             "open limit order"
#endif
    );
    return true;
}

bool ui_order(s_ui_ctx *ui_ctx, const s_action_metadata *metadata) {
    bool ret;
    const s_action *bulk_order;
    const s_order_request *limit_order;
    const s_action *update_leverage;
    const s_order_request *tp_order;
    const s_order_request *sl_order;

    if ((bulk_order = ctx_get_action(ACTION_TYPE_BULK_ORDER)) == NULL) {
        return false;
    }
    if ((limit_order = get_order_request(bulk_order->bulk_order.orders,
                                         bulk_order->bulk_order.order_count,
                                         &get_limit_request)) == NULL) {
        return false;
    }
    update_leverage = ctx_get_action(ACTION_TYPE_UPDATE_LEVERAGE);
    tp_order = get_order_request(bulk_order->bulk_order.orders,
                                 bulk_order->bulk_order.order_count,
                                 &get_trigger_tp);
    sl_order = get_order_request(bulk_order->bulk_order.orders,
                                 bulk_order->bulk_order.order_count,
                                 &get_trigger_sl);

    switch (limit_order->limit.tif) {
        case TIF_IOC:
            ret = ui_market_order(
                ui_ctx,
                metadata,
                limit_order,
                (update_leverage == NULL) ? NULL : &update_leverage->update_leverage,
                tp_order,
                sl_order);
            break;
        case TIF_GTC:
            ret =
                ui_limit_order(ui_ctx,
                               metadata,
                               limit_order,
                               (update_leverage == NULL) ? NULL : &update_leverage->update_leverage,
                               tp_order,
                               sl_order);
            break;
        default:
            ret = false;
    }
    return ret;
}
