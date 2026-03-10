#include "os_print.h"
#include "update_leverage.h"
#include "format.h"

static bool handle_asset(const tlv_data_t *data, s_update_leverage_ctx *out) {
    return get_uint32_t_from_tlv_data(data, &out->update_leverage->asset);
}

static bool handle_is_cross(const tlv_data_t *data, s_update_leverage_ctx *out) {
    return get_bool_from_tlv_data(data, &out->update_leverage->is_cross);
}

static bool handle_leverage(const tlv_data_t *data, s_update_leverage_ctx *out) {
    return get_uint32_t_from_tlv_data(data, &out->update_leverage->leverage);
}

#define UPDATE_LEVERAGE_TAGS(X)                                \
    X(0xd1, TAG_ASSET, handle_asset, ENFORCE_UNIQUE_TAG)       \
    X(0xde, TAG_IS_CROSS, handle_is_cross, ENFORCE_UNIQUE_TAG) \
    X(0xed, TAG_LEVERAGE, handle_leverage, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(UPDATE_LEVERAGE_TAGS, NULL, update_leverage_tlv_parser);

static bool verify_update_leverage(const s_update_leverage_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ASSET, TAG_IS_CROSS, TAG_LEVERAGE)) {
        PRINTF("Error: incomplete update_leverage struct received!\n");
        return false;
    }
    return true;
}

bool parse_update_leverage(const buffer_t *payload, s_update_leverage_ctx *out) {
    if (!update_leverage_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_update_leverage(out)) {
        return false;
    }
    return true;
}

void dump_update_leverage(const s_update_leverage *update_leverage) {
    (void) update_leverage;  // to prevent warnings for release builds
    PRINTF(">>> UPDATE_LEVERAGE >>>\n");
    PRINTF("asset = %u\n", update_leverage->asset);
    PRINTF("is_cross = %s\n", update_leverage->is_cross ? "true" : "false");
    PRINTF("leverage = %u\n", update_leverage->leverage);
    PRINTF("<<< UPDATE_LEVERAGE <<<\n");
}

bool update_leverage_serialize(const s_update_leverage *update_leverage, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 4)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "type", 4)) {
        return false;
    }
    if (!cmp_write_str(cmp_ctx, "updateLeverage", 14)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "asset", 5)) {
        return false;
    }
    if (!cmp_write_uinteger(cmp_ctx, update_leverage->asset)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "isCross", 7)) {
        return false;
    }
    if (!cmp_write_bool(cmp_ctx, update_leverage->is_cross)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "leverage", 8)) {
        return false;
    }
    if (!cmp_write_uinteger(cmp_ctx, update_leverage->leverage)) {
        return false;
    }
    return true;
}
