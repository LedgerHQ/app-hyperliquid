#pragma once

/* Hyperliquid TLV grammar dispatch table.
 *
 * Provides the per-command tlv_fuzz_config_t array consumed by
 * fuzz_tlv_dispatch_mutate() in the harness. fuzz_dispatcher.c uses this
 * to drive grammar-aware structural mutation for the metadata / action
 * APDUs. Commands that don't carry TLV payloads (GET_ADDRESS, SIGN_ACTION)
 * have empty entries that fall back to byte-level mutation.
 *
 * The actual emission of TLV entries lives in fuzz_tlv_config.c, which
 * shadows the SDK's tlv_custom_mutate() to use DER long-form for tag
 * and length values >= 0x80 (lib_tlv/tlv_library.c).
 */

#include "tlv_mutator.h"

extern const tlv_fuzz_config_t k_command_tlv_configs[];
extern const size_t            k_command_tlv_configs_count;
