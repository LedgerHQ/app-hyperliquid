#pragma once

#include "nbgl_content.h"
#include "op_order.h"

#define REVIEW_SIGN_STRING_LENGTH 128
#define MAX_UI_PAIRS              16

typedef struct {
    nbgl_contentTagValue_t pairs[MAX_UI_PAIRS];
    nbgl_contentTagValueList_t pair_list;

    char intent[REVIEW_SIGN_STRING_LENGTH + 1];
    char review_msg[REVIEW_SIGN_STRING_LENGTH + 1];
    char sign_msg[REVIEW_SIGN_STRING_LENGTH + 1];

    union {
        s_order_strings order;
    };
} s_ui_ctx;
