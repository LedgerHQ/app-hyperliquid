#pragma once

#include "constants.h"

typedef struct {
    // "Market" + " " + "Short"/"Long" + " - " + ASSET_TICKER_LENGTH
    char operation[LIMIT_MARKET_STRING_LENGTH + 1 + SHORT_LONG_STRING_LENGTH + 3 +
                   ASSET_TICKER_LENGTH + 1];

    char margin[MARGIN_STRING_LENGTH + 1];
    char leverage[LEVERAGE_STRING_LENGTH + 1];
    char size[SIZE_STRING_LENGTH + 1];
    char tp_price[PRICE_STRING_LENGTH + 1];
    char sl_price[PRICE_STRING_LENGTH + 1];
} s_modify_strings;
