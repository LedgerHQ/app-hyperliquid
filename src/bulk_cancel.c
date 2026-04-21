#include <string.h>
#include "os_print.h"
#include "os_utils.h"
#include "format.h"
#include "bulk_cancel.h"
#include "hl_context.h"

static bool handle_asset(const tlv_data_t *data, s_cancel_request_ctx *out) {
    return get_uint32_t_from_tlv_data(data, &out->cancel_request->asset);
}

static bool handle_oid(const tlv_data_t *data, s_cancel_request_ctx *out) {
    return get_uint64_t_from_tlv_data(data, &out->cancel_request->oid);
}

#define CANCEL_REQUEST_TAGS(X)                           \
    X(0xd1, TAG_ASSET, handle_asset, ENFORCE_UNIQUE_TAG) \
    X(0xdc, TAG_OID, handle_oid, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(CANCEL_REQUEST_TAGS, NULL, cancel_request_tlv_parser);

static bool verify_cancel_request(const s_cancel_request_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ASSET, TAG_OID)) {
        PRINTF("Error: incomplete cancel_request struct received!\n");
        return false;
    }
    if (ctx_get_action_metadata()->asset_id != out->cancel_request->asset) {
        PRINTF("Error: asset does not match metadata!\n");
        return false;
    }
    return true;
}

static bool parse_cancel_request(const buffer_t *payload, s_cancel_request_ctx *out) {
    if (!cancel_request_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_cancel_request(out)) {
        return false;
    }
    return true;
}

static void dump_cancel_request(const s_cancel_request *cancel_request) {
    char tmp[20 + 1];

    PRINTF(">>> CANCEL_REQUEST >>>\n");
    PRINTF("asset = %u\n", cancel_request->asset);
    format_u64(tmp, sizeof(tmp), cancel_request->oid);
    PRINTF("oid = %s\n", tmp);
    PRINTF("<<< CANCEL_REQUEST <<<\n");
}

static bool cancel_request_serialize(const s_cancel_request *cancel_request, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 2)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "a", 1)) {
        return false;
    }
    if (!cmp_write_uinteger(cmp_ctx, cancel_request->asset)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "o", 1)) {
        return false;
    }
    if (!cmp_write_uinteger(cmp_ctx, cancel_request->oid)) {
        return false;
    }
    return true;
}

static bool handle_cancel_request(const tlv_data_t *data, s_bulk_cancel_ctx *out) {
    if (out->bulk_cancel->cancel_count >= ARRAYLEN(out->bulk_cancel->cancels)) {
        PRINTF("Error: too many cancel requests to handle!\n");
        return false;
    }
    // wipe the sub-context since this tag can be handled multiple times
    explicit_bzero(&out->cancel_request_ctx, sizeof(out->cancel_request_ctx));

    out->cancel_request_ctx.cancel_request =
        &out->bulk_cancel->cancels[out->bulk_cancel->cancel_count];
    if (!parse_cancel_request(&data->value, &out->cancel_request_ctx)) {
        return false;
    }
    out->bulk_cancel->cancel_count += 1;
    return true;
}

#define BULK_CANCEL_TAGS(X) X(0xd9, TAG_CANCEL_REQUEST, handle_cancel_request, ALLOW_MULTIPLE_TAG)

DEFINE_TLV_PARSER(BULK_CANCEL_TAGS, NULL, bulk_cancel_tlv_parser);

static bool verify_bulk_cancel(const s_bulk_cancel_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_CANCEL_REQUEST)) {
        PRINTF("Error: incomplete bulk_cancel struct received!\n");
        return false;
    }
    return true;
}

bool parse_bulk_cancel(const buffer_t *payload, s_bulk_cancel_ctx *out) {
    if (!bulk_cancel_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_bulk_cancel(out)) {
        return false;
    }
    return true;
}

void dump_bulk_cancel(const s_bulk_cancel *bulk_cancel) {
    PRINTF(">>> BULK_CANCEL >>>\n");
    for (uint8_t i = 0; i < bulk_cancel->cancel_count; ++i) {
        dump_cancel_request(&bulk_cancel->cancels[i]);
    }
    PRINTF("<<< BULK_CANCEL <<<\n");
}

bool bulk_cancel_serialize(const s_bulk_cancel *bulk_cancel, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 2)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "type", 4)) {
        return false;
    }
    if (!cmp_write_str(cmp_ctx, "cancel", 6)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "cancels", 7)) {
        return false;
    }
    if (!cmp_write_array(cmp_ctx, bulk_cancel->cancel_count)) {
        return false;
    }
    for (uint8_t i = 0; i < bulk_cancel->cancel_count; ++i) {
        if (!cancel_request_serialize(&bulk_cancel->cancels[i], cmp_ctx)) {
            return false;
        }
    }
    return true;
}
