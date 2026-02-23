#pragma once

typedef enum {
    ACTION_TYPE_ORDER = 0x00,
    ACTION_TYPE_MODIFY = 0x01,
    ACTION_TYPE_CANCEL = 0x02,
    ACTION_TYPE_UPDATE_LEVERAGE = 0x03,
} e_action_type;
