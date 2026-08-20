#pragma once

/* Per-command TLV grammars consumed by fuzz_tlv_dispatch_mutate(). Commands
 * without a TLV payload have empty entries and fall back to byte mutation. */

#include "tlv_mutator.h"

extern const tlv_fuzz_config_t k_command_tlv_configs[];
extern const size_t            k_command_tlv_configs_count;
