#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "constants.h"

bool eip712_abstraction_hash(const uint64_t *chain_id,
                             const char *chain,
                             const uint8_t user[ADDRESS_LENGTH],
                             const char *abstraction,
                             const uint64_t *nonce,
                             uint8_t *domain_hash,
                             uint8_t *message_hash);
