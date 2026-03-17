#pragma once

#include "constants.h"

typedef struct {
    // "Cancel order - " + ASSET_TICKER_LENGTH
    char operation[15 + ASSET_TICKER_LENGTH + 1];

    char margin[MARGIN_STRING_LENGTH + 1];
    char leverage[LEVERAGE_STRING_LENGTH + 1];
} s_cancel_strings;
