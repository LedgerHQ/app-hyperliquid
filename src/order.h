#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "cmp.h"
#include "constants.h"

typedef enum {
    ORDER_TYPE_LIMIT = 0x00,
    ORDER_TYPE_TRIGGER = 0x01,
} e_order_type;

typedef enum {
    TIF_ALO = 0x00,
    TIF_IOC = 0x01,
    TIF_GTC = 0x02,
} e_tif;

typedef enum {
    TRIGGER_TYPE_TP = 0x00,
    TRIGGER_TYPE_SL = 0x01,
} e_trigger_type;

typedef struct {
    e_tif tif;
} s_limit;

typedef struct {
    bool is_market;
    char trigger_px[NUMERIC_STRING_LENGTH + 1];
    e_trigger_type trigger_type;
} s_trigger;

typedef struct {
    uint32_t asset;
    bool is_buy;
    char limit_px[NUMERIC_STRING_LENGTH + 1];
    char sz[NUMERIC_STRING_LENGTH + 1];
    bool reduce_only;
    e_order_type order_type;
    union {
        s_limit limit;
        s_trigger trigger;
    };
} s_order;

typedef struct {
    TLV_reception_t received_tags;
    s_order *order;
} s_order_ctx;

bool parse_order(const buffer_t *payload, s_order_ctx *out);
void dump_order(const s_order *order);
bool order_serialize(const s_order *order, cmp_ctx_t *cmp_ctx);
