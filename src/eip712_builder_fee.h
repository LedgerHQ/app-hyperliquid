#pragma once

#include <stdbool.h>
#include <stdint.h>

bool eip712_builder_fee_hash(const uint64_t *chain_id,
                             const char *chain,
                             const char *max_fee_rate,
                             const uint8_t *builder,
                             const uint64_t *nonce,
                             uint8_t *domain_hash,
                             uint8_t *message_hash);
