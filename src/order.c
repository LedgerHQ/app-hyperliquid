#include <string.h>
#include "os_print.h"
#include "tlv_library.h"
#include "order.h"

static bool handle_order_type(const tlv_data_t *data, s_order_ctx *out) {
    if (!get_uint8_t_from_tlv_data(data, &out->order->order_type)) {
        return false;
    }
    switch (out->order->order_type) {
        case ORDER_TYPE_LIMIT:
        case ORDER_TYPE_TRIGGER:
            break;
        default:
            return false;
    }
    return true;
}

static bool handle_asset(const tlv_data_t *data, s_order_ctx *out) {
    return get_uint32_t_from_tlv_data(data, &out->order->asset);
}

static bool handle_is_buy(const tlv_data_t *data, s_order_ctx *out) {
    return get_bool_from_tlv_data(data, &out->order->is_buy);
}

static bool handle_limit_px(const tlv_data_t *data, s_order_ctx *out) {
    return get_string_from_tlv_data(data, out->order->limit_px, 1, sizeof(out->order->limit_px));
}

static bool handle_sz(const tlv_data_t *data, s_order_ctx *out) {
    return get_string_from_tlv_data(data, out->order->sz, 1, sizeof(out->order->sz));
}

static bool handle_reduce_only(const tlv_data_t *data, s_order_ctx *out) {
    return get_bool_from_tlv_data(data, &out->order->reduce_only);
}

// forward declarations because these functions access an enum declared by the parser definition
static bool handle_tif(const tlv_data_t *, s_order_ctx *);
static bool handle_is_market(const tlv_data_t *data, s_order_ctx *out);
static bool handle_trigger_px(const tlv_data_t *data, s_order_ctx *out);
static bool handle_trigger_type(const tlv_data_t *data, s_order_ctx *out);

#define ORDER_TAGS(X)                                                \
    X(0xe0, TAG_ORDER_TYPE, handle_order_type, ENFORCE_UNIQUE_TAG)   \
    X(0xe1, TAG_ASSET, handle_asset, ENFORCE_UNIQUE_TAG)             \
    X(0xe2, TAG_IS_BUY, handle_is_buy, ENFORCE_UNIQUE_TAG)           \
    X(0xe3, TAG_LIMIT_PX, handle_limit_px, ENFORCE_UNIQUE_TAG)       \
    X(0xe4, TAG_SZ, handle_sz, ENFORCE_UNIQUE_TAG)                   \
    X(0xe5, TAG_REDUCE_ONLY, handle_reduce_only, ENFORCE_UNIQUE_TAG) \
    X(0xe6, TAG_TIF, handle_tif, ENFORCE_UNIQUE_TAG)                 \
    X(0xe7, TAG_IS_MARKET, handle_is_market, ENFORCE_UNIQUE_TAG)     \
    X(0xe8, TAG_TRIGGER_PX, handle_trigger_px, ENFORCE_UNIQUE_TAG)   \
    X(0xe9, TAG_TRIGGER_TYPE, handle_trigger_type, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(ORDER_TAGS, NULL, order_tlv_parser);

static bool handle_tif(const tlv_data_t *data, s_order_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ORDER_TYPE) ||
        (out->order->order_type != ORDER_TYPE_LIMIT)) {
        return false;
    }
    if (!get_uint8_t_from_tlv_data(data, &out->order->limit.tif)) {
        return false;
    }
    switch (out->order->limit.tif) {
        case TIF_ALO:
        case TIF_IOC:
        case TIF_GTC:
            break;
        default:
            return false;
    }
    return true;
}

static bool handle_is_market(const tlv_data_t *data, s_order_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ORDER_TYPE) ||
        (out->order->order_type != ORDER_TYPE_TRIGGER)) {
        return false;
    }
    return get_bool_from_tlv_data(data, &out->order->trigger.is_market);
}

static bool handle_trigger_px(const tlv_data_t *data, s_order_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ORDER_TYPE) ||
        (out->order->order_type != ORDER_TYPE_TRIGGER)) {
        return false;
    }
    return get_string_from_tlv_data(data,
                                    out->order->trigger.trigger_px,
                                    1,
                                    sizeof(out->order->trigger.trigger_px));
}

static bool handle_trigger_type(const tlv_data_t *data, s_order_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ORDER_TYPE) ||
        (out->order->order_type != ORDER_TYPE_TRIGGER)) {
        return false;
    }
    if (!get_uint8_t_from_tlv_data(data, &out->order->trigger.trigger_type)) {
        return false;
    }
    switch (out->order->trigger.trigger_type) {
        case TRIGGER_TYPE_TP:
        case TRIGGER_TYPE_SL:
            break;
        default:
            return false;
    }
    return true;
}

static bool verify_order(s_order_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags,
                                 TAG_ORDER_TYPE,
                                 TAG_ASSET,
                                 TAG_IS_BUY,
                                 TAG_LIMIT_PX,
                                 TAG_SZ,
                                 TAG_REDUCE_ONLY)) {
        PRINTF("Error: incomplete order struct received!\n");
        return false;
    }
    // Now check dynamically required tags
    switch (out->order->order_type) {
        case ORDER_TYPE_LIMIT:
            if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_TIF)) {
                PRINTF("Error: incomplete limit order struct received!\n");
                return false;
            }
            break;
        case ORDER_TYPE_TRIGGER:
            if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags,
                                         TAG_IS_MARKET,
                                         TAG_TRIGGER_PX,
                                         TAG_TRIGGER_TYPE)) {
                PRINTF("Error: incomplete trigger order struct received!\n");
                return false;
            }
            break;
    }
    return true;
}

void dump_order(const s_order *order) {
    PRINTF(">>> ORDER >>>\n");
    PRINTF("asset = %u\n", order->asset);
    PRINTF("is_buy = %s\n", order->is_buy ? "true" : "false");
    PRINTF("limit_px = \"%s\"\n", order->limit_px);
    PRINTF("sz = \"%s\"\n", order->sz);
    PRINTF("reduce_only = %s\n", order->reduce_only ? "true" : "false");
    switch (order->order_type) {
        case ORDER_TYPE_LIMIT:
            PRINTF("tif = %u\n", order->limit.tif);
            break;
        case ORDER_TYPE_TRIGGER:
            PRINTF("is_market = %s\n", order->trigger.is_market ? "true" : "false");
            PRINTF("trigger_px = \"%s\"\n", order->trigger.trigger_px);
            PRINTF("trigger_type = %u\n", order->trigger.trigger_type);
            break;
    }
    PRINTF("<<< ORDER <<<\n");
}

bool parse_order(const buffer_t *payload, s_order_ctx *out) {
    if (!order_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_order(out)) {
        return false;
    }
    return true;
}

static bool order_limit_serialize(const s_limit *limit, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_str(cmp_ctx, "limit", 5)) {
        return false;
    }
    if (!cmp_write_map(cmp_ctx, 1)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "tif", 3)) {
        return false;
    }
    switch (limit->tif) {
        case TIF_ALO:
            if (!cmp_write_str(cmp_ctx, "Alo", 3)) {
                return false;
            }
            break;
        case TIF_IOC:
            if (!cmp_write_str(cmp_ctx, "Ioc", 3)) {
                return false;
            }
            break;
        case TIF_GTC:
            if (!cmp_write_str(cmp_ctx, "Gtc", 3)) {
                return false;
            }
            break;
    }
    return true;
}

static bool order_trigger_serialize(const s_trigger *trigger, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_str(cmp_ctx, "trigger", 7)) {
        return false;
    }
    if (!cmp_write_map(cmp_ctx, 3)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "isMarket", 8)) {
        return false;
    }
    if (!cmp_write_bool(cmp_ctx, trigger->is_market)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "triggerPx", 9)) {
        return false;
    }
    if (!cmp_write_str(cmp_ctx, trigger->trigger_px, strlen(trigger->trigger_px))) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "triggerType", 11)) {
        return false;
    }
    switch (trigger->trigger_type) {
        case TRIGGER_TYPE_TP:
            if (!cmp_write_str(cmp_ctx, "tp", 2)) {
                return false;
            }
            break;
        case TRIGGER_TYPE_SL:
            if (!cmp_write_str(cmp_ctx, "sl", 2)) {
                return false;
            }
            break;
    }
    return true;
}

bool order_serialize(const s_order *order, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 6)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "a", 1)) {
        return false;
    }
    if (!cmp_write_uinteger(cmp_ctx, order->asset)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "b", 1)) {
        return false;
    }
    if (!cmp_write_bool(cmp_ctx, order->is_buy)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "p", 1)) {
        return false;
    }
    if (!cmp_write_str(cmp_ctx, order->limit_px, strlen(order->limit_px))) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "s", 1)) {
        return false;
    }
    if (!cmp_write_str(cmp_ctx, order->sz, strlen(order->sz))) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "r", 1)) {
        return false;
    }
    if (!cmp_write_bool(cmp_ctx, order->reduce_only)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "t", 1)) {
        return false;
    }
    if (!cmp_write_map(cmp_ctx, 1)) {
        return false;
    }

    switch (order->order_type) {
        case ORDER_TYPE_LIMIT:
            if (!order_limit_serialize(&order->limit, cmp_ctx)) {
                return false;
            }
            break;
        case ORDER_TYPE_TRIGGER:
            if (!order_trigger_serialize(&order->trigger, cmp_ctx)) {
                return false;
            }
            break;
    }
    return true;
}
