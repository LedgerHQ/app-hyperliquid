#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "buffer.h"
#include "constants.h"

#define ASSET_TICKER_LENGTH 48

typedef enum {
    OP_TYPE_ORDER = 0x00,
    OP_TYPE_MODIFY = 0x01,
    OP_TYPE_CANCEL = 0x02,
    OP_TYPE_UPDATE_LEVERAGE = 0x03,
    OP_TYPE_CLOSE = 0x04,
    OP_TYPE_UPDATE_MARGIN = 0x05,
} e_operation_type;

typedef enum {
    NETWORK_MAINNET = 0,
    NETWORK_TESTNET = 1,
} e_network;

typedef struct {
    e_operation_type op_type;
    uint32_t asset_id;
    char asset_ticker[ASSET_TICKER_LENGTH + 1];
    e_network network;
    bool has_builder_addr;
    uint8_t builder_addr[ADDRESS_LENGTH];
    bool has_margin;
    uint64_t margin;
    bool has_leverage;
    uint32_t leverage;
} s_action_metadata;

bool parse_action_metadata(const buffer_t *payload);
