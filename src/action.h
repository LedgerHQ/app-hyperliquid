#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "buffer.h"
#include "bulk_order.h"
#include "bulk_modify.h"
#include "bulk_cancel.h"
#include "update_leverage.h"
#include "approve_builder_fee.h"
#include "update_isolated_margin.h"
#include "user_set_abstraction.h"
#include "action_metadata.h"
#include "constants.h"

typedef enum {
    ACTION_TYPE_BULK_ORDER = 0x00,
    ACTION_TYPE_BULK_MODIFY = 0x01,
    ACTION_TYPE_BULK_CANCEL = 0x02,
    ACTION_TYPE_UPDATE_LEVERAGE = 0x03,
    ACTION_TYPE_APPROVE_BUILDER_FEE = 0x04,
    ACTION_TYPE_UPDATE_ISOLATED_MARGIN = 0x05,
    ACTION_TYPE_USER_SET_ABSTRACTION = 0x06,
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
        s_update_isolated_margin update_isolated_margin;
        s_user_set_abstraction user_set_abstraction;
    };
} s_action;

bool parse_action(const buffer_t *payload);
bool action_hash(const s_action *action,
                 const s_action_metadata *metadata,
                 const uint8_t wallet_addr[ADDRESS_LENGTH],
                 uint8_t *domain_hash,
                 uint8_t *message_hash);
