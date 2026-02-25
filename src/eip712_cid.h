#pragma once

#include "eip712_common.h"

bool eip712_cid_hash(const char *source,
                     const uint8_t *cid,
                     uint8_t *domain_hash,
                     uint8_t *message_hash);
