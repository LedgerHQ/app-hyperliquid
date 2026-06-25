#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "tlv_library.h"
#include "constants.h"

typedef enum {
    ABSTRACTION_DISABLED = 0x00,
    ABSTRACTION_UNIFIED_ACCOUNT = 0x01,
    ABSTRACTION_PORTFOLIO_MARGIN = 0x02,
} e_abstraction;

typedef struct {
    uint64_t signature_chain_id;
    e_abstraction abstraction;
} s_user_set_abstraction;

typedef struct {
    TLV_reception_t received_tags;
    s_user_set_abstraction *user_set_abstraction;
} s_user_set_abstraction_ctx;

bool parse_user_set_abstraction(const buffer_t *payload, s_user_set_abstraction_ctx *out);
void dump_user_set_abstraction(const s_user_set_abstraction *user_set_abstraction);
const char *get_abstraction_string(const s_user_set_abstraction *user_set_abstraction);
