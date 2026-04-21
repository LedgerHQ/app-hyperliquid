#include <stdio.h>
#include "format.h"
#include "ui_common.h"

void show_metadata_margin(s_ui_ctx *ui_ctx,
                          const s_action_metadata *metadata,
                          char *out,
                          size_t size) {
    // bigger value than necessary until the SDK function is fixed
    char tmp[NUMERIC_STRING_LENGTH + MARGIN_DECIMALS + 1] = {0};

    if (metadata->has_margin) {
        format_fpu64_trimmed(tmp, sizeof(tmp), metadata->margin, MARGIN_DECIMALS);
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Margin";
        snprintf(out, size, "%s %s", tmp, COUNTERVALUE_TICKER);
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = out;
        ui_ctx->pair_list.nbPairs += 1;
    }
}

void show_metadata_leverage(s_ui_ctx *ui_ctx,
                            const s_action_metadata *metadata,
                            char *out,
                            size_t size) {
    if (metadata->has_leverage) {
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Leverage";
        snprintf(out, size, "%ux", metadata->leverage);
        ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = out;
        ui_ctx->pair_list.nbPairs += 1;
    }
}

const s_order_request *get_order_request(const s_order_request *list,
                                         size_t size,
                                         f_order_request_matcher match_func) {
    for (size_t i = 0; i < size; ++i) {
        if (match_func(&list[i])) {
            return &list[i];
        }
    }
    return NULL;
}

size_t count_order_requests(const s_order_request *list,
                            size_t size,
                            f_order_request_matcher match_func) {
    size_t count = 0;

    for (size_t i = 0; i < size; ++i) {
        if (match_func(&list[i])) {
            count++;
        }
    }
    return count;
}

bool get_limit_request(const s_order_request *req) {
    return req->order_type == ORDER_TYPE_LIMIT;
}

bool get_trigger_tp(const s_order_request *req) {
    return (req->order_type == ORDER_TYPE_TRIGGER) && (req->trigger.tpsl == TRIGGER_TYPE_TP);
}

bool get_trigger_sl(const s_order_request *req) {
    return (req->order_type == ORDER_TYPE_TRIGGER) && (req->trigger.tpsl == TRIGGER_TYPE_SL);
}

static const char *short_long_string(const s_order_request *req,
                                     const char *short_str,
                                     const char *long_str) {
    if (req != NULL) {
        if (req->reduce_only) {
            return (req->is_buy) ? short_str : long_str;
        } else {
            return (req->is_buy) ? long_str : short_str;
        }
    }
    return NULL;
}

const char *get_short_long_string(const s_order_request *req) {
    return short_long_string(req, "short", "long");
}

const char *get_short_long_string_capitalized(const s_order_request *req) {
    return short_long_string(req, "Short", "Long");
}
