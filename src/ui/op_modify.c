#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "action.h"
#include "hl_context.h"
#include "ui_context.h"
#include "ui_common.h"

typedef bool (*f_modify_request_matcher)(const s_modify_request *);

static const s_modify_request *get_modify_request(const s_modify_request *list,
                                                  size_t size,
                                                  f_modify_request_matcher match_func) {
    for (size_t i = 0; i < size; ++i) {
        if (match_func(&list[i])) {
            return &list[i];
        }
    }
    return NULL;
}

static bool get_modify_tp(const s_modify_request *req) {
    return (req->order.order_type == ORDER_TYPE_TRIGGER) &&
           (req->order.trigger.tpsl == TRIGGER_TYPE_TP);
}

static bool get_modify_sl(const s_modify_request *req) {
    return (req->order.order_type == ORDER_TYPE_TRIGGER) &&
           (req->order.trigger.tpsl == TRIGGER_TYPE_SL);
}

bool ui_modify(s_ui_ctx *ui_ctx, const s_action_metadata *metadata) {
    const s_action *action;
    const s_modify_request *mod_tp_req;
    const s_modify_request *mod_sl_req;
    const s_order_request *tp_req = NULL;
    const s_order_request *sl_req = NULL;
    const s_order_request *any_req;
    char target_tmp[32];

    if ((action = ctx_get_action(ACTION_TYPE_BULK_MODIFY)) == NULL) {
        action = ctx_get_action(ACTION_TYPE_BULK_ORDER);
    }
    if (action == NULL) {
        return false;
    }
    switch (action->type) {
        case ACTION_TYPE_BULK_MODIFY:
            if ((mod_tp_req = get_modify_request(action->bulk_modify.modifies,
                                                 action->bulk_modify.modify_count,
                                                 &get_modify_tp)) != NULL) {
                tp_req = &mod_tp_req->order;
            }
            if ((mod_sl_req = get_modify_request(action->bulk_modify.modifies,
                                                 action->bulk_modify.modify_count,
                                                 &get_modify_sl)) != NULL) {
                sl_req = &mod_sl_req->order;
            }
            break;

        case ACTION_TYPE_BULK_ORDER:
            tp_req = get_order_request(action->bulk_order.orders,
                                       action->bulk_order.order_count,
                                       &get_trigger_tp);
            sl_req = get_order_request(action->bulk_order.orders,
                                       action->bulk_order.order_count,
                                       &get_trigger_sl);
            break;

        default:
            return false;
    }

    if ((tp_req == NULL) && (sl_req == NULL)) {
        return false;
    }
    any_req = (tp_req == NULL) ? sl_req : tp_req;

    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Active Position";
    snprintf(ui_ctx->modify.operation,
             sizeof(ui_ctx->modify.operation),
             "Market %s - %s",
             get_short_long_string_capitalized(any_req),
             metadata->asset_ticker);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->modify.operation;
    ui_ctx->pair_list.nbPairs += 1;

    show_metadata_margin(ui_ctx, metadata, ui_ctx->modify.margin, sizeof(ui_ctx->modify.margin));
    show_metadata_leverage(ui_ctx,
                           metadata,
                           ui_ctx->modify.leverage,
                           sizeof(ui_ctx->modify.leverage));

    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Size";
    snprintf(ui_ctx->modify.size,
             sizeof(ui_ctx->modify.size),
             "%s %s",
             any_req->sz,
             metadata->asset_ticker);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->modify.size;
    ui_ctx->pair_list.nbPairs += 1;

    if (tp_req != NULL) {
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item =
            (action->type == ACTION_TYPE_BULK_MODIFY) ? "New Take Profit" : "Take Profit";
        snprintf(ui_ctx->modify.tp_price,
                 sizeof(ui_ctx->modify.tp_price),
                 "%s %s",
                 tp_req->trigger.trigger_px,
                 COUNTERVALUE_TICKER);
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->modify.tp_price;
        ui_ctx->pair_list.nbPairs += 1;
    } else {
        // only SL
        snprintf(target_tmp,
                 sizeof(target_tmp),
#ifdef SCREEN_SIZE_WALLET
                 "Stop Loss"
#else
                 "SL"
#endif
        );
    }

    if (sl_req != NULL) {
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item =
            (action->type == ACTION_TYPE_BULK_MODIFY) ? "New Stop Loss" : "Stop Loss";
        snprintf(ui_ctx->modify.sl_price,
                 sizeof(ui_ctx->modify.sl_price),
                 "%s %s",
                 sl_req->trigger.trigger_px,
                 COUNTERVALUE_TICKER);
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->modify.sl_price;
        ui_ctx->pair_list.nbPairs += 1;
    } else {
        // only TP
        snprintf(target_tmp,
                 sizeof(target_tmp),
#ifdef SCREEN_SIZE_WALLET
                 "Take Profit"
#else
                 "TP"
#endif
        );
    }

    if ((tp_req != NULL) && (sl_req != NULL)) {
        // both
        snprintf(target_tmp,
                 sizeof(target_tmp),
#ifdef SCREEN_SIZE_WALLET
                 "Take Profit and Stop Loss"
#else
                 "TP/SL"
#endif
        );
    }

    snprintf(ui_ctx->intent,
             sizeof(ui_ctx->intent),
#ifdef SCREEN_SIZE_WALLET
             "%s %s %s",
             (action->type == ACTION_TYPE_BULK_ORDER) ? "Set" : "Edit",
             metadata->asset_ticker,
             target_tmp
#else
             "%s %s",
             (action->type == ACTION_TYPE_BULK_ORDER) ? "set" : "edit",
             target_tmp
#endif
    );
    return true;
}
