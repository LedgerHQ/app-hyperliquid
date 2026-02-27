#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "cmp.h"

typedef struct {
    uint32_t asset;
    uint64_t oid;
} s_cancel_request;

typedef struct {
    s_cancel_request cancels;
} s_bulk_cancel;

typedef struct {
    TLV_reception_t received_tags;
    s_bulk_cancel *bulk_cancel;
} s_bulk_cancel_ctx;

bool parse_bulk_cancel(const buffer_t *payload, s_bulk_cancel_ctx *out);
void dump_bulk_cancel(const s_bulk_cancel *bulk_cancel);
bool bulk_cancel_serialize(const s_bulk_cancel *bulk_cancel, cmp_ctx_t *cmp_ctx);
