#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "buffer.h"
#include "bulk_order.h"
#include "bulk_modify.h"
#include "bulk_cancel.h"
#include "update_leverage.h"
#include "approve_builder_fee.h"
#include "action_metadata.h"

typedef enum {
    ACTION_TYPE_ORDER = 0x00,
    ACTION_TYPE_MODIFY = 0x01,
    ACTION_TYPE_CANCEL = 0x02,
    ACTION_TYPE_UPDATE_LEVERAGE = 0x03,
    ACTION_TYPE_APPROVE_BUILDER_FEE = 0x04,
} e_action_type;

typedef struct {
    e_action_type type;
    uint64_t nonce;
    union {
        s_bulk_order bulk_order;
        s_bulk_modify bulk_modify;
        s_bulk_cancel bulk_cancel;
        s_update_leverage update_leverage;
        s_approve_builder_fee approve_builder_fee;
    };
} s_action;

bool parse_action(const buffer_t *payload);
bool action_hash(const s_action *action,
                 const s_action_metadata *metadata,
                 uint8_t *domain_hash,
                 uint8_t *message_hash);
