#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "constants.h"

bool derive_addr_from_path(const uint32_t *bip32_path,
                           uint8_t path_length,
                           uint8_t out[ADDRESS_LENGTH]);
