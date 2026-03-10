#include "os_print.h"
#include "io.h"
#include "status_words.h"
#include "nbgl_use_case.h"
#include "format.h"
#include "display_action.h"
#include "hl_context.h"
#include "menu.h"
#include "sign_action.h"

#ifdef SCREEN_SIZE_WALLET
#define APP_REVIEW_ICON LARGE_REVIEW_ICON
#else
#define APP_REVIEW_ICON REVIEW_ICON
#endif

#define REVIEW_SIGN_STRING_LENGTH 128
#define MAX_UI_PAIRS              16

static nbgl_contentTagValue_t g_pairs[MAX_UI_PAIRS];
static nbgl_contentTagValueList_t g_pair_list;

static void review_choise(bool confirm) {
    if (confirm) {
        if (sign_action()) {
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

typedef struct {
    char review[REVIEW_SIGN_STRING_LENGTH + 1];
    char sign[REVIEW_SIGN_STRING_LENGTH + 1];
    union {
        struct {
            // "Limit"/"Market" + " " + "Short"/"Long" + " - " + ASSET_TICKER_LENGTH
            char operation[6 + 1 + 5 + 3 + ASSET_TICKER_LENGTH + 1];

            // NUMERIC_STRING_LENGTH + ASSET_TICKER_LENGTH
            char limit_price[NUMERIC_STRING_LENGTH + ASSET_TICKER_LENGTH + 1];

            // MAX_UINT32(4294967295) + "x"
            char leverage[10 + 1 + 1];

            // NUMERIC_STRING_LENGTH + " " + ASSET_TICKER_LENGTH
            char margin[NUMERIC_STRING_LENGTH + 1 + ASSET_TICKER_LENGTH + 1];

            // NUMERIC_STRING_LENGTH + " " + ASSET_TICKER_LENGTH
            char tp_price[NUMERIC_STRING_LENGTH + 1 + ASSET_TICKER_LENGTH + 1];

            // NUMERIC_STRING_LENGTH + " " + ASSET_TICKER_LENGTH
            char sl_price[NUMERIC_STRING_LENGTH + 1 + ASSET_TICKER_LENGTH + 1];

            // NUMERIC_STRING_LENGTH + " " + ASSET_TICKER_LENGTH
            char size[NUMERIC_STRING_LENGTH + 1 + ASSET_TICKER_LENGTH + 1];
        } order;

        struct {
        } modify;
    };
} s_ui_strings;

static s_ui_strings g_ui_strings;

static bool ui_order_common(const s_action_metadata *metadata,
                            const s_order_request *limit,
                            const s_update_leverage *update_leverage,
                            const s_order_request *tp_order,
                            const s_order_request *sl_order) {
    if (metadata->has_margin) {
        g_pairs[g_pair_list.nbPairs].item = "Margin";
        snprintf(g_ui_strings.order.margin,
                 sizeof(g_ui_strings.order.margin),
                 "%s %s",
                 metadata->margin,
                 metadata->asset_ticker);
        g_pairs[g_pair_list.nbPairs].value = g_ui_strings.order.margin;
        g_pair_list.nbPairs += 1;
    }

    if (update_leverage != NULL) {
        g_pairs[g_pair_list.nbPairs].item = "Leverage";
        snprintf(g_ui_strings.order.leverage,
                 sizeof(g_ui_strings.order.leverage),
                 "%ux",
                 update_leverage->leverage);
        g_pairs[g_pair_list.nbPairs].value = g_ui_strings.order.leverage;
        g_pair_list.nbPairs += 1;
    }

    if (tp_order != NULL) {
        g_pairs[g_pair_list.nbPairs].item = "Take Profit";
        snprintf(g_ui_strings.order.tp_price,
                 sizeof(g_ui_strings.order.tp_price),
                 "%s %s",
                 tp_order->trigger.trigger_px,
                 "USDC");
        g_pairs[g_pair_list.nbPairs].value = g_ui_strings.order.tp_price;
        g_pair_list.nbPairs += 1;
    }

    if (sl_order != NULL) {
        g_pairs[g_pair_list.nbPairs].item = "Stop Loss";
        snprintf(g_ui_strings.order.sl_price,
                 sizeof(g_ui_strings.order.sl_price),
                 "%s %s",
                 sl_order->trigger.trigger_px,
                 "USDC");
        g_pairs[g_pair_list.nbPairs].value = g_ui_strings.order.sl_price;
        g_pair_list.nbPairs += 1;
    }

    g_pairs[g_pair_list.nbPairs].item = "Size";
    snprintf(g_ui_strings.order.size,
             sizeof(g_ui_strings.order.size),
             "%s %s",
             limit->sz,
             metadata->asset_ticker);
    g_pairs[g_pair_list.nbPairs].value = g_ui_strings.order.size;
    g_pair_list.nbPairs += 1;

    return true;
}

static bool ui_market_order(const s_action_metadata *metadata,
                            const s_order_request *limit,
                            const s_update_leverage *update_leverage,
                            const s_order_request *tp_order,
                            const s_order_request *sl_order) {
    g_pairs[g_pair_list.nbPairs].item = "Operation";
    snprintf(g_ui_strings.order.operation,
             sizeof(g_ui_strings.order.operation),
             "Market %s - %s",
             limit->is_buy ? "Long" : "Short",
             metadata->asset_ticker);
    g_pairs[g_pair_list.nbPairs].value = g_ui_strings.order.operation;
    g_pair_list.nbPairs += 1;

    if (!ui_order_common(metadata, limit, update_leverage, tp_order, sl_order)) {
        return false;
    }

    snprintf(g_ui_strings.review,
             sizeof(g_ui_strings.review),
#ifdef SCREEN_SIZE_WALLET
             "Review message to open %s %s",
             metadata->asset_ticker,
             limit->is_buy ? "long" : "short"
#else
             "Review message to open %s",
             limit->is_buy ? "long" : "short"
#endif
    );
    snprintf(g_ui_strings.sign,
             sizeof(g_ui_strings.sign),
#ifdef SCREEN_SIZE_WALLET
             "Sign message to open %s %s",
             metadata->asset_ticker,
             limit->is_buy ? "long" : "short"
#else
             "Sign message to open %s",
             limit->is_buy ? "long" : "short"
#endif
    );

    return true;
}

static bool ui_limit_order(const s_action_metadata *metadata,
                           const s_order_request *limit,
                           const s_update_leverage *update_leverage,
                           const s_order_request *tp_order,
                           const s_order_request *sl_order) {
    g_pairs[g_pair_list.nbPairs].item = "Operation";
    snprintf(g_ui_strings.order.operation,
             sizeof(g_ui_strings.order.operation),
             "Limit %s - %s",
             limit->is_buy ? "Long" : "Short",
             metadata->asset_ticker);
    g_pairs[g_pair_list.nbPairs].value = g_ui_strings.order.operation;
    g_pair_list.nbPairs += 1;

    g_pairs[g_pair_list.nbPairs].item = "Limit Price";
    snprintf(g_ui_strings.order.limit_price,
             sizeof(g_ui_strings.order.limit_price),
             "%s %s",
             limit->limit_px,
             "USDC");
    g_pairs[g_pair_list.nbPairs].value = g_ui_strings.order.limit_price;
    g_pair_list.nbPairs += 1;

    if (!ui_order_common(metadata, limit, update_leverage, tp_order, sl_order)) {
        return false;
    }

    snprintf(g_ui_strings.review,
             sizeof(g_ui_strings.review),
#ifdef SCREEN_SIZE_WALLET
             "Review message to open %s limit order",
             metadata->asset_ticker
#else
             "Review message to open limit order"
#endif
    );
    snprintf(g_ui_strings.sign,
             sizeof(g_ui_strings.sign),
#ifdef SCREEN_SIZE_WALLET
             "Sign message to open %s limit order",
             metadata->asset_ticker
#else
             "Sign message to open limit order"
#endif
    );

    return true;
}

typedef bool (*f_order_request_matcher)(const s_order_request *);

static const s_order_request *get_order_request(const s_order_request *list,
                                                size_t size,
                                                f_order_request_matcher match_func) {
    for (size_t i = 0; i < size; ++i) {
        if (match_func(&list[i])) {
            return &list[i];
        }
    }
    return NULL;
}

static bool get_limit_request(const s_order_request *req) {
    return req->order_type == ORDER_TYPE_LIMIT;
}

static bool get_trigger_tp(const s_order_request *req) {
    return (req->order_type == ORDER_TYPE_TRIGGER) && (req->trigger.tpsl == TRIGGER_TYPE_TP);
}

static bool get_trigger_sl(const s_order_request *req) {
    return (req->order_type == ORDER_TYPE_TRIGGER) && (req->trigger.tpsl == TRIGGER_TYPE_SL);
}

static bool ui_order(const s_action_metadata *metadata) {
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
                metadata,
                limit_order,
                (update_leverage == NULL) ? NULL : &update_leverage->update_leverage,
                tp_order,
                sl_order);
            break;
        case TIF_GTC:
            ret =
                ui_limit_order(metadata,
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

static bool ui_modify(const s_action_metadata *metadata) {
    (void) metadata;
    // TODO
    return true;
}

bool handle_ui(const s_action_metadata *metadata) {
    bool ret;

    if (metadata == NULL) {
        return false;
    }
    explicit_bzero(&g_pair_list, sizeof(g_pair_list));
    explicit_bzero(&g_pairs, sizeof(g_pairs));
    explicit_bzero(&g_ui_strings, sizeof(g_ui_strings));
    g_pair_list.pairs = (nbgl_contentTagValue_t *) &g_pairs;

    g_pairs[g_pair_list.nbPairs].item = "Protocol";
    g_pairs[g_pair_list.nbPairs].value = APPNAME;
    g_pair_list.nbPairs += 1;

    switch (metadata->op_type) {
        case OP_TYPE_ORDER:
            ret = ui_order(metadata);
            break;
        case OP_TYPE_MODIFY:
            ret = ui_modify(metadata);
            break;
        default:
            PRINTF("Error: no UI flow for given operation type (%u)!\n", metadata->op_type);
            ret = false;
    }
    if (ret) {
        nbgl_useCaseReview(TYPE_MESSAGE,
                           &g_pair_list,
                           &APP_REVIEW_ICON,
                           g_ui_strings.review,
                           NULL,
                           g_ui_strings.sign,
                           &review_choise);
    }
    return ret;
}
