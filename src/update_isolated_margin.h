#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "cmp.h"
#include "constants.h"

typedef struct {
    uint32_t asset;
    bool is_buy;
    int64_t ntli;
} s_update_isolated_margin;

typedef struct {
    TLV_reception_t received_tags;
    s_update_isolated_margin *update_isolated_margin;
} s_update_isolated_margin_ctx;

bool parse_update_isolated_margin(const buffer_t *payload, s_update_isolated_margin_ctx *out);
void dump_update_isolated_margin(const s_update_isolated_margin *update_isolated_margin);
bool update_isolated_margin_serialize(const s_update_isolated_margin *update_isolated_margin,
                                      cmp_ctx_t *cmp_ctx);
