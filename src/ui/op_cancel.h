#pragma once

#include "constants.h"

#include "bulk_cancel.h"

typedef struct {
    // "Cancel order - " + ASSET_TICKER_LENGTH + " " + "Take Profit"/"Stop Loss"/
    // "Take Profit and Stop Loss"
    char operation[15 + ASSET_TICKER_LENGTH + 1 + 25 + 1];

    char margin[MARGIN_STRING_LENGTH + 1];
    char leverage[LEVERAGE_STRING_LENGTH + 1];

    // oid is signed but otherwise hidden, so each cancelled order id is shown explicitly;
    // 20 digits max for uint64
    char oid[BULK_MAX_SIZE][20 + 1];
} s_cancel_strings;
