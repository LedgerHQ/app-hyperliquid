#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "cmp.h"
#include "order.h"
#include "constants.h"

#define BULK_ORDER_MAX_ORDER_COUNT 5

typedef enum {
    GROUPING_NA = 0x00,
    GROUPING_NORMAL_TPSL = 0x01,
    GROUPING_POSITION_TPSL = 0x02,
} e_grouping;

typedef struct {
    uint8_t builder[ADDRESS_LENGTH];
    uint64_t fee;
} s_builder_info;

typedef struct {
    uint8_t order_count;
    s_order orders[BULK_ORDER_MAX_ORDER_COUNT];
    e_grouping grouping;
    bool has_builder;
    s_builder_info builder;
} s_bulk_order;

typedef struct {
    TLV_reception_t received_tags;
    s_bulk_order *bulk_order;
    s_order_ctx order_ctx;
} s_bulk_order_ctx;

bool parse_bulk_order(const buffer_t *payload, s_bulk_order_ctx *out);
void dump_bulk_order(const s_bulk_order *bulk_order);
bool bulk_order_serialize(const s_bulk_order *bulk_order, cmp_ctx_t *cmp_ctx);
