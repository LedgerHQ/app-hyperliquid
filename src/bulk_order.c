#include <string.h>
#include "os_print.h"
#include "tlv_library.h"
#include "bulk_order.h"
#include "format.h"    // format_u64
#include "os_utils.h"  // bytes_to_lowercase_hex

static bool handle_builder_address(const tlv_data_t *data, s_builder_info_ctx *out) {
    buffer_t buf;

    if (!get_buffer_from_tlv_data(data,
                                  &buf,
                                  sizeof(out->builder_info->builder),
                                  sizeof(out->builder_info->builder))) {
        return false;
    }
    memcpy(out->builder_info->builder, buf.ptr, sizeof(out->builder_info->builder));
    return true;
}

static bool handle_builder_fee(const tlv_data_t *data, s_builder_info_ctx *out) {
    return get_uint64_t_from_tlv_data(data, &out->builder_info->fee);
}

#define BUILDER_INFO_TAGS(X)                                                 \
    X(0xd3, TAG_BUILDER_ADDRESS, handle_builder_address, ENFORCE_UNIQUE_TAG) \
    X(0xec, TAG_BUILDER_FEE, handle_builder_fee, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(BUILDER_INFO_TAGS, NULL, builder_info_tlv_parser);

static bool verify_builder_info(const s_builder_info_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_BUILDER_ADDRESS, TAG_BUILDER_FEE)) {
        PRINTF("Error: incomplete builder_info struct received!\n");
        return false;
    }
    return true;
}

static void dump_builder_info(const s_builder_info *builder_info) {
    char tmp[20 + 1];

    PRINTF(">>> BUILDER_INFO >>>\n");
    PRINTF("address = 0x%.*h\n", sizeof(builder_info->builder), builder_info->builder);
    format_u64(tmp, sizeof(tmp), builder_info->fee);
    PRINTF("fee = %s\n", tmp);
    PRINTF("<<< BUILDER_INFO <<<\n");
}

static bool parse_builder_info(const buffer_t *payload, s_builder_info_ctx *out) {
    if (!builder_info_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_builder_info(out)) {
        return false;
    }
    return true;
}

static bool builder_serialize(const s_builder_info *builder, cmp_ctx_t *cmp_ctx) {
    char builder_addr_str[2 + (ADDRESS_LENGTH * 2) + 1];

    if (!cmp_write_str(cmp_ctx, "builder", 7)) {
        return false;
    }
    if (!cmp_write_map(cmp_ctx, 2)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "b", 1)) {
        return false;
    }
    memcpy(builder_addr_str, "0x", 2);
    bytes_to_lowercase_hex(&builder_addr_str[2],
                           sizeof(builder_addr_str) - 2,
                           builder->builder,
                           sizeof(builder->builder));
    if (!cmp_write_str(cmp_ctx, builder_addr_str, strlen(builder_addr_str))) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "f", 1)) {
        return false;
    }
    if (!cmp_write_uinteger(cmp_ctx, builder->fee)) {
        return false;
    }
    return true;
}

static bool handle_order(const tlv_data_t *data, s_bulk_order_ctx *out) {
    if (out->bulk_order->order_count >= ARRAYLEN(out->bulk_order->orders)) {
        PRINTF("Error: too many orders to handle!\n");
        return false;
    }
    // wipe the sub-context since this tag can be handled multiple times
    explicit_bzero(&out->order_ctx, sizeof(out->order_ctx));

    out->order_ctx.order_request = &out->bulk_order->orders[out->bulk_order->order_count];
    if (!parse_order_request(&data->value, &out->order_ctx)) {
        return false;
    }
    out->bulk_order->order_count += 1;
    return true;
}

static bool handle_grouping(const tlv_data_t *data, s_bulk_order_ctx *out) {
    if (!get_uint8_t_from_tlv_data(data, &out->bulk_order->grouping)) {
        return false;
    }
    switch (out->bulk_order->grouping) {
        case GROUPING_NA:
        case GROUPING_NORMAL_TPSL:
        case GROUPING_POSITION_TPSL:
            break;
        default:
            return false;
    }
    return true;
}

static bool handle_builder(const tlv_data_t *data, s_bulk_order_ctx *out) {
    out->builder_info_ctx.builder_info = &out->bulk_order->builder;
    if (!parse_builder_info(&data->value, &out->builder_info_ctx)) {
        return false;
    }
    out->bulk_order->has_builder = true;
    return true;
}

#define BULK_ORDER_TAGS(X)                                     \
    X(0xdd, TAG_ORDER, handle_order, ALLOW_MULTIPLE_TAG)       \
    X(0xea, TAG_GROUPING, handle_grouping, ENFORCE_UNIQUE_TAG) \
    X(0xeb, TAG_BUILDER, handle_builder, ENFORCE_UNIQUE_TAG)

DEFINE_TLV_PARSER(BULK_ORDER_TAGS, NULL, bulk_order_tlv_parser);

static bool verify_bulk_order(const s_bulk_order_ctx *out) {
    if (!TLV_CHECK_RECEIVED_TAGS(out->received_tags, TAG_ORDER, TAG_GROUPING)) {
        PRINTF("Error: incomplete bulk_order struct received!\n");
        return false;
    }
    return true;
}

void dump_bulk_order(const s_bulk_order *bulk_order) {
    PRINTF(">>> BULK_ORDER >>>\n");
    for (uint8_t i = 0; i < bulk_order->order_count; ++i) {
        dump_order_request(&bulk_order->orders[i]);
    }
    PRINTF("grouping = %u\n", bulk_order->grouping);
    if (bulk_order->has_builder) {
        dump_builder_info(&bulk_order->builder);
    }
    PRINTF("<<< BULK_ORDER <<<\n");
}

bool parse_bulk_order(const buffer_t *payload, s_bulk_order_ctx *out) {
    if (!bulk_order_tlv_parser(payload, out, &out->received_tags)) {
        return false;
    }
    if (!verify_bulk_order(out)) {
        return false;
    }
    return true;
}

bool bulk_order_serialize(const s_bulk_order *bulk_order, cmp_ctx_t *cmp_ctx) {
    if (!cmp_write_map(cmp_ctx, bulk_order->has_builder ? 4 : 3)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "type", 4)) {
        return false;
    }
    if (!cmp_write_str(cmp_ctx, "order", 5)) {
        return false;
    }

    if (!cmp_write_str(cmp_ctx, "orders", 6)) {
        return false;
    }
    if (!cmp_write_array(cmp_ctx, bulk_order->order_count)) {
        return false;
    }

    for (uint8_t i = 0; i < bulk_order->order_count; ++i) {
        if (!order_request_serialize(&bulk_order->orders[i], cmp_ctx)) {
            return false;
        }
    }

    if (!cmp_write_str(cmp_ctx, "grouping", 8)) {
        return false;
    }
    switch (bulk_order->grouping) {
        case GROUPING_NA:
            if (!cmp_write_str(cmp_ctx, "na", 2)) {
                return false;
            }
            break;
        case GROUPING_NORMAL_TPSL:
            if (!cmp_write_str(cmp_ctx, "normalTpsl", 10)) {
                return false;
            }
            break;
        case GROUPING_POSITION_TPSL:
            if (!cmp_write_str(cmp_ctx, "positionTpsl", 12)) {
                return false;
            }
            break;
    }

    if (bulk_order->has_builder) {
        if (!builder_serialize(&bulk_order->builder, cmp_ctx)) {
            return false;
        }
    }
    return true;
}
