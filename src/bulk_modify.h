#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "cmp.h"
#include "order_request.h"
#include "constants.h"

typedef struct {
    uint64_t oid;
    s_order_request order;
} s_modify_request;

typedef struct {
    TLV_reception_t received_tags;
    s_modify_request *modify_request;
    s_order_request_ctx order_ctx;
} s_modify_request_ctx;

typedef struct {
    uint8_t modify_count;
    s_modify_request modifies[BULK_MAX_SIZE];
} s_bulk_modify;

typedef struct {
    TLV_reception_t received_tags;
    s_bulk_modify *bulk_modify;
    s_modify_request_ctx modify_request_ctx;
} s_bulk_modify_ctx;

bool parse_bulk_modify(const buffer_t *payload, s_bulk_modify_ctx *out);
void dump_bulk_modify(const s_bulk_modify *bulk_modify);
bool bulk_modify_serialize(const s_bulk_modify *bulk_modify, cmp_ctx_t *cmp_ctx);
