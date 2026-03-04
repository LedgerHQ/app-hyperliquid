#include <string.h>
#include "os_print.h"
#include "tlv_library.h"
#include "order_request.h"

static bool handle_tif(const tlv_data_t *data, s_limit_ctx *out) {
    if (!get_uint8_t_from_tlv_data(data, &out->limit->tif)) {
        return false;
    }
    switch (out->limit->tif) {
        case TIF_ALO:
        case TIF_IOC:
        case TIF_GTC:
            break;
        default:
            return false;
    }
    return true;
}

#define LIMIT_ORDER_TAGS(X) X(0xe6, TAG_TIF, handle_tif, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(LIMIT_ORDER_TAGS, NULL, limit_order_tlv_parser);

static bool verify_limit_order(const s_limit_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_TIF)) {
        PRINTF("Error: incomplete limit struct received!\n");
        return false;
    }
    return true;
}

static void dump_limit_order(const s_limit *limit) {
    (void) limit;  // to prevent warnings for release builds
    PRINTF(">>> LIMIT >>>\n");
    PRINTF("tif = %u\n", limit->tif);
    PRINTF("<<< LIMIT <<<\n");
}

static bool parse_limit_order(const buffer_t *payload, s_limit_ctx *out) {
    if (!limit_order_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_limit_order(out)) {
        return false;
    }
    return true;
}

static bool limit_order_serialize(const s_limit *limit, cmp_ctx_t *cmp_ctx) {
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

static bool handle_is_market(const tlv_data_t *data, s_trigger_ctx *out) {
    return get_bool_from_tlv_data(data, &out->trigger->is_market);
}

static bool handle_trigger_px(const tlv_data_t *data, s_trigger_ctx *out) {
    return get_string_from_tlv_data(data,
                                    out->trigger->trigger_px,
                                    1,
                                    sizeof(out->trigger->trigger_px));
}

static bool handle_tpsl(const tlv_data_t *data, s_trigger_ctx *out) {
    if (!get_uint8_t_from_tlv_data(data, &out->trigger->tpsl)) {
        return false;
    }
    switch (out->trigger->tpsl) {
        case TRIGGER_TYPE_TP:
        case TRIGGER_TYPE_SL:
            break;
        default:
            return false;
    }
    return true;
}

#define TRIGGER_ORDER_TAGS(X)                                      \
    X(0xe7, TAG_IS_MARKET, handle_is_market, ENFORCE_UNIQUE_TAG)   \
    X(0xe8, TAG_TRIGGER_PX, handle_trigger_px, ENFORCE_UNIQUE_TAG) \
    X(0xe9, TAG_TRIGGER_TYPE, handle_tpsl, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(TRIGGER_ORDER_TAGS, NULL, trigger_order_tlv_parser);

static bool verify_trigger_order(const s_trigger_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags,
                                 TAG_IS_MARKET,
                                 TAG_TRIGGER_PX,
                                 TAG_TRIGGER_TYPE)) {
        PRINTF("Error: incomplete trigger struct received!\n");
        return false;
    }
    return true;
}

static void dump_trigger_order(const s_trigger *trigger) {
    (void) trigger;  // to prevent warnings for release builds
    PRINTF(">>> TRIGGER >>>\n");
    PRINTF("is_market = %s\n", trigger->is_market ? "true" : "false");
    PRINTF("trigger_px = \"%s\"\n", trigger->trigger_px);
    PRINTF("tpsl = %u\n", trigger->tpsl);
    PRINTF("<<< TRIGGER <<<\n");
}

static bool trigger_order_serialize(const s_trigger *trigger, cmp_ctx_t *cmp_ctx) {
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
    switch (trigger->tpsl) {
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

static bool parse_trigger_order(const buffer_t *payload, s_trigger_ctx *out) {
    if (!trigger_order_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_trigger_order(out)) {
        return false;
    }
    return true;
}

static bool handle_order_type(const tlv_data_t *data, s_order_request_ctx *out) {
    if (!get_uint8_t_from_tlv_data(data, &out->order_request->order_type)) {
        return false;
    }
    switch (out->order_request->order_type) {
        case ORDER_TYPE_LIMIT:
        case ORDER_TYPE_TRIGGER:
            break;
        default:
            return false;
    }
    return true;
}

static bool handle_asset(const tlv_data_t *data, s_order_request_ctx *out) {
    return get_uint32_t_from_tlv_data(data, &out->order_request->asset);
}

static bool handle_is_buy(const tlv_data_t *data, s_order_request_ctx *out) {
    return get_bool_from_tlv_data(data, &out->order_request->is_buy);
}

static bool handle_limit_px(const tlv_data_t *data, s_order_request_ctx *out) {
    return get_string_from_tlv_data(data,
                                    out->order_request->limit_px,
                                    1,
                                    sizeof(out->order_request->limit_px));
}

static bool handle_sz(const tlv_data_t *data, s_order_request_ctx *out) {
    return get_string_from_tlv_data(data,
                                    out->order_request->sz,
                                    1,
                                    sizeof(out->order_request->sz));
}

static bool handle_reduce_only(const tlv_data_t *data, s_order_request_ctx *out) {
    return get_bool_from_tlv_data(data, &out->order_request->reduce_only);
}

static bool handle_order(const tlv_data_t *, s_order_request_ctx *);

#define ORDER_REQUEST_TAGS(X)                                        \
    X(0xe0, TAG_ORDER_TYPE, handle_order_type, ENFORCE_UNIQUE_TAG)   \
    X(0xe1, TAG_ASSET, handle_asset, ENFORCE_UNIQUE_TAG)             \
    X(0xe2, TAG_IS_BUY, handle_is_buy, ENFORCE_UNIQUE_TAG)           \
    X(0xe3, TAG_LIMIT_PX, handle_limit_px, ENFORCE_UNIQUE_TAG)       \
    X(0xe4, TAG_SZ, handle_sz, ENFORCE_UNIQUE_TAG)                   \
    X(0xe5, TAG_REDUCE_ONLY, handle_reduce_only, ENFORCE_UNIQUE_TAG) \
    X(0xd7, TAG_ORDER, handle_order, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(ORDER_REQUEST_TAGS, NULL, order_request_tlv_parser);

static bool handle_order(const tlv_data_t *data, s_order_request_ctx *out) {
    bool ret;

    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ORDER_TYPE)) {
        return false;
    }
    switch (out->order_request->order_type) {
        case ORDER_TYPE_LIMIT:
            out->limit_ctx.limit = &out->order_request->limit;
            ret = parse_limit_order(&data->value, &out->limit_ctx);
            break;
        case ORDER_TYPE_TRIGGER:
            out->trigger_ctx.trigger = &out->order_request->trigger;
            ret = parse_trigger_order(&data->value, &out->trigger_ctx);
            break;
        default:
            ret = false;
    }
    return ret;
}

static bool verify_order_request(const s_order_request_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags,
                                 TAG_ORDER_TYPE,
                                 TAG_ASSET,
                                 TAG_IS_BUY,
                                 TAG_LIMIT_PX,
                                 TAG_SZ,
                                 TAG_REDUCE_ONLY,
                                 TAG_ORDER)) {
        PRINTF("Error: incomplete order_request struct received!\n");
        return false;
    }
    return true;
}

void dump_order_request(const s_order_request *order_request) {
    PRINTF(">>> ORDER >>>\n");
    PRINTF("asset = %u\n", order_request->asset);
    PRINTF("is_buy = %s\n", order_request->is_buy ? "true" : "false");
    PRINTF("limit_px = \"%s\"\n", order_request->limit_px);
    PRINTF("sz = \"%s\"\n", order_request->sz);
    PRINTF("reduce_only = %s\n", order_request->reduce_only ? "true" : "false");
    switch (order_request->order_type) {
        case ORDER_TYPE_LIMIT:
            dump_limit_order(&order_request->limit);
            break;
        case ORDER_TYPE_TRIGGER:
            dump_trigger_order(&order_request->trigger);
            break;
    }
    PRINTF("<<< ORDER <<<\n");
}

bool parse_order_request(const buffer_t *payload, s_order_request_ctx *out) {
    if (!order_request_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_order_request(out)) {
        return false;
    }
    return true;
}

bool order_request_serialize(const s_order_request *order_request, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 6)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "a", 1)) {
        return false;
    }
    if (!cmp_write_uinteger(cmp_ctx, order_request->asset)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "b", 1)) {
        return false;
    }
    if (!cmp_write_bool(cmp_ctx, order_request->is_buy)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "p", 1)) {
        return false;
    }
    if (!cmp_write_str(cmp_ctx, order_request->limit_px, strlen(order_request->limit_px))) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "s", 1)) {
        return false;
    }
    if (!cmp_write_str(cmp_ctx, order_request->sz, strlen(order_request->sz))) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "r", 1)) {
        return false;
    }
    if (!cmp_write_bool(cmp_ctx, order_request->reduce_only)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "t", 1)) {
        return false;
    }
    if (!cmp_write_map(cmp_ctx, 1)) {
        return false;
    }

    switch (order_request->order_type) {
        case ORDER_TYPE_LIMIT:
            if (!limit_order_serialize(&order_request->limit, cmp_ctx)) {
                return false;
            }
            break;
        case ORDER_TYPE_TRIGGER:
            if (!trigger_order_serialize(&order_request->trigger, cmp_ctx)) {
                return false;
            }
            break;
        default:
            return false;
    }
    return true;
}
