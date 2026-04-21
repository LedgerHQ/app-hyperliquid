#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "cmp.h"
#include "constants.h"

typedef struct {
    uint32_t asset;
    uint64_t oid;
} s_cancel_request;

typedef struct {
    TLV_reception_t received_tags;
    s_cancel_request *cancel_request;
} s_cancel_request_ctx;

typedef struct {
    uint8_t cancel_count;
    s_cancel_request cancels[BULK_MAX_SIZE];
} s_bulk_cancel;

typedef struct {
    TLV_reception_t received_tags;
    s_bulk_cancel *bulk_cancel;
    s_cancel_request_ctx cancel_request_ctx;
} s_bulk_cancel_ctx;

bool parse_bulk_cancel(const buffer_t *payload, s_bulk_cancel_ctx *out);
void dump_bulk_cancel(const s_bulk_cancel *bulk_cancel);
bool bulk_cancel_serialize(const s_bulk_cancel *bulk_cancel, cmp_ctx_t *cmp_ctx);
