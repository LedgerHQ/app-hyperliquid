#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "buffer.h"
#include "action_type.h"
#include "bulk_order.h"
#include "bulk_modify.h"

typedef struct {
    e_action_type type;
    uint64_t nonce;
    union {
        s_bulk_order bulk_order;
        s_bulk_modify bulk_modify;
    };
} s_action;

bool parse_action(const buffer_t *payload);
bool action_hash(const s_action *action, uint8_t *domain_hash, uint8_t *message_hash);
