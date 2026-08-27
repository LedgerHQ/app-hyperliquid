#pragma once

#include "constants.h"
#include "bulk_modify.h"

typedef struct {
    // "Market" + " " + "Short"/"Long" + " - " + ASSET_TICKER_LENGTH
    char operation[LIMIT_MARKET_STRING_LENGTH + 1 + SHORT_LONG_STRING_LENGTH + 3 +
                   ASSET_TICKER_LENGTH + 1];

    char margin[MARGIN_STRING_LENGTH + 1];
    char leverage[LEVERAGE_STRING_LENGTH + 1];
    char size[SIZE_STRING_LENGTH + 1];
    char tp_price[PRICE_STRING_LENGTH + 1];
    char sl_price[PRICE_STRING_LENGTH + 1];

    // oid is signed but otherwise hidden, so each modified order id is shown explicitly;
    // 20 digits max for uint64
    char tp_oid[20 + 1];
    char sl_oid[20 + 1];
} s_modify_strings;
