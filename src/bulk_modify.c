#include <string.h>
#include "os_print.h"
#include "os_utils.h"
#include "bulk_modify.h"
#include "format.h"

static bool handle_order(const tlv_data_t *data, s_modify_request_ctx *out) {
    out->order_ctx.order_request = &out->modify_request->order;
    return parse_order_request(&data->value, &out->order_ctx);
}

static bool handle_oid(const tlv_data_t *data, s_modify_request_ctx *out) {
    return get_uint64_t_from_tlv_data(data, &out->modify_request->oid);
}

#define MODIFY_REQUEST_TAGS(X)                           \
    X(0xdd, TAG_ORDER, handle_order, ENFORCE_UNIQUE_TAG) \
    X(0xdc, TAG_OID, handle_oid, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(MODIFY_REQUEST_TAGS, NULL, modify_request_tlv_parser);

static bool verify_modify_request(const s_modify_request_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ORDER, TAG_OID)) {
        PRINTF("Error: incomplete modify_request struct received!\n");
        return false;
    }
    return true;
}

static bool parse_modify_request(const buffer_t *payload, s_modify_request_ctx *out) {
    if (!modify_request_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_modify_request(out)) {
        return false;
    }
    return true;
}

static void dump_modify_request(const s_modify_request *modify_request) {
    char tmp[20 + 1];

    PRINTF(">>> MODIFY_REQUEST >>>\n");
    dump_order_request(&modify_request->order);
    format_u64(tmp, sizeof(tmp), modify_request->oid);
    PRINTF("oid = %s\n", tmp);
    PRINTF("<<< MODIFY_REQUEST <<<\n");
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
    if (!order_request_serialize(&modify_request->order, cmp_ctx)) {
        return false;
    }
    return true;
}

static bool handle_modify_request(const tlv_data_t *data, s_bulk_modify_ctx *out) {
    if (out->bulk_modify->modify_count >= ARRAYLEN(out->bulk_modify->modifies)) {
        PRINTF("Error: too many modify requests to handle!\n");
        return false;
    }
    // wipe the sub-context since this tag can be handled multiple times
    explicit_bzero(&out->modify_request_ctx, sizeof(out->modify_request_ctx));

    out->modify_request_ctx.modify_request =
        &out->bulk_modify->modifies[out->bulk_modify->modify_count];
    if (!parse_modify_request(&data->value, &out->modify_request_ctx)) {
        return false;
    }
    out->bulk_modify->modify_count += 1;
    return true;
}

#define BULK_MODIFY_TAGS(X) X(0xd8, TAG_MODIFY_REQUEST, handle_modify_request, ALLOW_MULTIPLE_TAG)

DEFINE_TLV_PARSER(BULK_MODIFY_TAGS, NULL, bulk_modify_tlv_parser);

static bool verify_bulk_modify(const s_bulk_modify_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_MODIFY_REQUEST)) {
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
    PRINTF(">>> BULK_MODIFY >>>\n");
    for (uint8_t i = 0; i < bulk_modify->modify_count; ++i) {
        dump_modify_request(&bulk_modify->modifies[i]);
    }
    PRINTF("<<< BULK_MODIFY <<<\n");
}

bool bulk_modify_serialize(const s_bulk_modify *bulk_modify, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, 1)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "modifies", 8)) {
        return false;
    }
    if (!cmp_write_array(cmp_ctx, bulk_modify->modify_count)) {
        return false;
    }
    for (uint8_t i = 0; i < bulk_modify->modify_count; ++i) {
        if (!modify_request_serialize(&bulk_modify->modifies[i], cmp_ctx)) {
            return false;
        }
    }
    return true;
}
