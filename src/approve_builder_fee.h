#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "constants.h"

typedef struct {
    uint64_t signature_chain_id;
    char max_fee_rate[NUMERIC_STRING_LENGTH + 1];
    uint8_t builder[ADDRESS_LENGTH];
} s_approve_builder_fee;

typedef struct {
    TLV_reception_t received_tags;
    s_approve_builder_fee *approve_builder_fee;
} s_approve_builder_fee_ctx;

bool parse_approve_builder_fee(const buffer_t *payload, s_approve_builder_fee_ctx *out);
void dump_approve_builder_fee(const s_approve_builder_fee *approve_builder_fee);
