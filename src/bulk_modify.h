#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "cmp.h"
#include "order.h"

typedef struct {
    uint64_t oid;
    s_order order;
} s_modify_request;

typedef struct {
    s_modify_request modifies;
} s_bulk_modify;

typedef struct {
    TLV_reception_t received_tags;
    s_bulk_modify *bulk_modify;
    s_order_ctx order_ctx;
} s_bulk_modify_ctx;

bool parse_bulk_modify(const buffer_t *payload, s_bulk_modify_ctx *out);
void dump_bulk_modify(const s_bulk_modify *bulk_modify);
bool bulk_modify_serialize(const s_bulk_modify *bulk_modify, cmp_ctx_t *cmp_ctx);
