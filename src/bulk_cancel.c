#include "os_print.h"
#include "bulk_cancel.h"
#include "format.h"

static bool handle_asset(const tlv_data_t *data, s_bulk_cancel_ctx *out) {
    return get_uint32_t_from_tlv_data(data, &out->bulk_cancel->cancels.asset);
}

static bool handle_oid(const tlv_data_t *data, s_bulk_cancel_ctx *out) {
    return get_uint64_t_from_tlv_data(data, &out->bulk_cancel->cancels.oid);
}

#define BULK_CANCEL_TAGS(X)                              \
    X(0xd1, TAG_ASSET, handle_asset, ENFORCE_UNIQUE_TAG) \
    X(0xdc, TAG_OID, handle_oid, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(BULK_CANCEL_TAGS, NULL, bulk_cancel_tlv_parser);

static bool verify_bulk_cancel(s_bulk_cancel_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ASSET, TAG_OID)) {
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
    char tmp[20 + 1];

    PRINTF(">>> BULK_CANCEL >>>\n");
    PRINTF("asset = %u\n", bulk_cancel->cancels.asset);
    format_u64(tmp, sizeof(tmp), bulk_cancel->cancels.oid);
    PRINTF("oid = %s\n", tmp);
    PRINTF("<<< BULK_CANCEL <<<\n");
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

bool bulk_cancel_serialize(const s_bulk_cancel *bulk_cancel, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 1)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "cancels", 7)) {
        return false;
    }
    if (!cmp_write_array(cmp_ctx, 1)) {
        return false;
    }
    if (!cancel_request_serialize(&bulk_cancel->cancels, cmp_ctx)) {
        return false;
    }
    return true;
}
