#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "cmp.h"

typedef struct {
    uint32_t asset;
    bool is_cross;
    uint32_t leverage;
} s_update_leverage;

typedef struct {
    TLV_reception_t received_tags;
    s_update_leverage *update_leverage;
} s_update_leverage_ctx;

bool parse_update_leverage(const buffer_t *payload, s_update_leverage_ctx *out);
void dump_update_leverage(const s_update_leverage *update_leverage);
bool update_leverage_serialize(const s_update_leverage *update_leverage, cmp_ctx_t *cmp_ctx);
