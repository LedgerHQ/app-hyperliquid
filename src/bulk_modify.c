#include "os_print.h"
#include "bulk_modify.h"
#include "format.h"

static bool handle_order(const tlv_data_t *data, s_bulk_modify_ctx *out) {
    out->order_ctx.order = &out->bulk_modify->modifies.order;
    return parse_order(&data->value, &out->order_ctx);
}

static bool handle_oid(const tlv_data_t *data, s_bulk_modify_ctx *out) {
    return get_uint64_t_from_tlv_data(data, &out->bulk_modify->modifies.oid);
}

#define BULK_MODIFY_TAGS(X)                              \
    X(0xdd, TAG_ORDER, handle_order, ENFORCE_UNIQUE_TAG) \
    X(0xdc, TAG_OID, handle_oid, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(BULK_MODIFY_TAGS, NULL, bulk_modify_tlv_parser);

static bool verify_bulk_modify(s_bulk_modify_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ORDER, TAG_OID)) {
        PRINTF("Error: incomplete bulk_modify struct received!\n");
        return false;
    }
    return true;
}

bool parse_bulk_modify(const buffer_t *payload, s_bulk_modify_ctx *out) {
    if (!bulk_modify_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_bulk_modify(out)) {
        return false;
    }
    return true;
}

void dump_bulk_modify(const s_bulk_modify *bulk_modify) {
    char tmp[20 + 1];

    PRINTF(">>> BULK_MODIFY >>>\n");
    dump_order(&bulk_modify->modifies.order);
    format_u64(tmp, sizeof(tmp), bulk_modify->modifies.oid);
    PRINTF("oid = %s\n", tmp);
    PRINTF("<<< BULK_MODIFY <<<\n");
}

static bool modify_request_serialize(const s_modify_request *modify_request, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 2)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "oid", 3)) {
        return false;
    }
    if (!cmp_write_uinteger(cmp_ctx, modify_request->oid)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "order", 5)) {
        return false;
    }
    if (!order_serialize(&modify_request->order, cmp_ctx)) {
        return false;
    }
    return true;
}

bool bulk_modify_serialize(const s_bulk_modify *bulk_modify, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 1)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "modifies", 8)) {
        return false;
    }
    if (!cmp_write_array(cmp_ctx, 1)) {
        return false;
    }
    if (!modify_request_serialize(&bulk_modify->modifies, cmp_ctx)) {
        return false;
    }
    return true;
}
