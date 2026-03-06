#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "buffer.h"
#include "action_type.h"
#include "constants.h"

#define ASSET_TICKER_LENGTH 48

typedef enum {
    NETWORK_MAINNET = 0,
    NETWORK_TESTNET = 1,
} e_network;

typedef struct {
    e_action_type action_type;
    uint32_t asset_id;
    char asset_ticker[ASSET_TICKER_LENGTH + 1];
    e_network network;
    bool has_builder_addr;
    uint8_t builder_addr[ADDRESS_LENGTH];
    bool has_margin;
    char margin[NUMERIC_STRING_LENGTH + 1];
} s_action_metadata;

bool parse_action_metadata(const buffer_t *payload);
