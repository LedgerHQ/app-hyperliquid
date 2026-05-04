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
    TLV_reception_t received_tags;
    s_limit *limit;
} s_limit_ctx;

typedef struct {
    bool is_market;
    char trigger_px[NUMERIC_STRING_LENGTH + 1];
    e_trigger_type tpsl;
} s_trigger;

typedef struct {
    TLV_reception_t received_tags;
    s_trigger *trigger;
} s_trigger_ctx;

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
    bool has_cloid;
    char cloid[CLOID_HEX_STRING_LENGTH];
} s_order_request;

typedef struct {
    TLV_reception_t received_tags;
    s_order_request *order_request;
    union {
        s_limit_ctx limit_ctx;
        s_trigger_ctx trigger_ctx;
    };
} s_order_request_ctx;

bool parse_order_request(const buffer_t *payload, s_order_request_ctx *out);
void dump_order_request(const s_order_request *order_request);
bool order_request_serialize(const s_order_request *order_request, cmp_ctx_t *cmp_ctx);
