#include <string.h>
#include "os_print.h"
#include "format.h"
#include "update_isolated_margin.h"

static bool handle_asset(const tlv_data_t *data, s_update_isolated_margin_ctx *out) {
    return get_uint32_t_from_tlv_data(data, &out->update_isolated_margin->asset);
}

static bool handle_is_buy(const tlv_data_t *data, s_update_isolated_margin_ctx *out) {
    return get_bool_from_tlv_data(data, &out->update_isolated_margin->is_buy);
}

static bool handle_ntli(const tlv_data_t *data, s_update_isolated_margin_ctx *out) {
    uint64_t tmp;

    if (!get_uint64_t_from_tlv_data(data, &tmp)) {
        return false;
    }
    memcpy(&out->update_isolated_margin->ntli, &tmp, sizeof(tmp));
    return true;
}

#define UPDATE_ISOLATED_MARGIN_TAGS(X)                     \
    X(0xd1, TAG_ASSET, handle_asset, ENFORCE_UNIQUE_TAG)   \
    X(0xe2, TAG_IS_BUY, handle_is_buy, ENFORCE_UNIQUE_TAG) \
    X(0xd6, TAG_NTLI, handle_ntli, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(UPDATE_ISOLATED_MARGIN_TAGS, NULL, update_isolated_margin_tlv_parser);

static bool verify_update_isolated_margin(const s_update_isolated_margin_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ASSET, TAG_IS_BUY, TAG_NTLI)) {
        PRINTF("Error: incomplete update_isolated_margin struct received!\n");
        return false;
    }
    return true;
}

bool parse_update_isolated_margin(const buffer_t *payload, s_update_isolated_margin_ctx *out) {
    if (!update_isolated_margin_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_update_isolated_margin(out)) {
        return false;
    }
    return true;
}

void dump_update_isolated_margin(const s_update_isolated_margin *update_isolated_margin) {
    char tmp[20 + 1];

    (void) update_isolated_margin;  // to prevent warnings for release builds
    PRINTF(">>> UPDATE_ISOLATED_MARGIN >>>\n");
    PRINTF("asset = %u\n", update_isolated_margin->asset);
    PRINTF("is_buy = %s\n", update_isolated_margin->is_buy ? "true" : "false");
    format_i64(tmp, sizeof(tmp), update_isolated_margin->ntli);
    PRINTF("ntli = %s\n", tmp);
    PRINTF("<<< UPDATE_ISOLATED_MARGIN <<<\n");
}

bool update_isolated_margin_serialize(const s_update_isolated_margin *update_isolated_margin,
                                      cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 4)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "type", 4)) {
        return false;
    }
    if (!cmp_write_str(cmp_ctx, "updateIsolatedMargin", 20)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "asset", 5)) {
        return false;
    }
    if (!cmp_write_uinteger(cmp_ctx, update_isolated_margin->asset)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "isBuy", 5)) {
        return false;
    }
    if (!cmp_write_bool(cmp_ctx, update_isolated_margin->is_buy)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "ntli", 4)) {
        return false;
    }
    if (!cmp_write_integer(cmp_ctx, update_isolated_margin->ntli)) {
        return false;
    }
    return true;
}
