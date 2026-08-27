/* Hyperliquid per-INS TLV grammars, indexed in fuzz_commands[] order and
 * handed to fuzz_tlv_dispatch_mutate(). */

#include "fuzz_tlv_config.h"

#include <stddef.h>
#include <stdint.h>

#include "tlv_mutator.h"

/*  ─── Per-INS TLV grammars ─────────────────────────────────────────────── */

/* INS_PROVIDE_ACTION_METADATA payload, parsed by src/action_metadata.c. Value
 * lengths match the get_*_from_tlv_data helpers plus the size checks in
 * handle_*(). Tags are stored raw; the mutator applies the DER framing. */
static const tlv_tag_info_t TAGS_ACTION_METADATA[] = {
    {0x01, 1,  1 },   /* TAG_STRUCT_TYPE      (uint8, must equal 0x2b)   */
    {0x02, 1,  1 },   /* TAG_STRUCT_VERSION   (uint8, must equal 1)      */
    {0xd0, 1,  1 },   /* TAG_OPERATION_TYPE   (uint8, e_operation_type)  */
    {0xd1, 4,  4 },   /* TAG_ASSET_ID         (uint32)                   */
    {0x24, 1,  48},   /* TAG_ASSET_TICKER     (string, 1..TICKER_LEN+1)  */
    {0xd2, 1,  1 },   /* TAG_NETWORK          (uint8, e_network)         */
    {0xd3, 20, 20},   /* TAG_BUILDER_ADDR     (fixed 20-byte address)    */
    {0xd4, 8,  8 },   /* TAG_MARGIN           (uint64)                   */
    {0xd5, 4,  4 },   /* TAG_LEVERAGE         (uint32)                   */
    {0x15, 70, 72},   /* TAG_SIGNATURE        (DER-encoded ECDSA sig)    */
};

/* INS_SET_ACTION payload — parsed by src/action.c (action_tlv_parser).
 * TAG_ACTION carries a nested TLV whose grammar varies per action_type;
 * we let the inner level be filled by libFuzzer byte-level mutation
 * plus the action sub-tag tokens declared in fuzz-manifest.toml
 * dictionary. Length kept conservative so build_complete can fit it. */
static const tlv_tag_info_t TAGS_ACTION[] = {
    {0x01, 1,   1  }, /* TAG_STRUCT_TYPE      (uint8, must equal 0x2c)   */
    {0x02, 1,   1  }, /* TAG_STRUCT_VERSION   (uint8, must equal 1)      */
    {0xd0, 1,   1  }, /* TAG_ACTION_TYPE      (uint8, e_action_type)     */
    {0xda, 8,   8  }, /* TAG_NONCE            (uint64)                   */
    {0xdb, 2,   200}, /* TAG_ACTION           (nested TLV blob)          */
};

/* Indexed by command in fuzz_commands[] order (see fuzz_dispatcher.c).
 * Entries with num_tags == 0 fall back to the byte-level mutator. */
const tlv_fuzz_config_t k_command_tlv_configs[] = {
    [0] = {0},                            /* INS_GET_ADDRESS              — BIP32 */
    [1] = TLV_CFG(TAGS_ACTION_METADATA),  /* INS_PROVIDE_ACTION_METADATA       */
    [2] = TLV_CFG(TAGS_ACTION),           /* INS_SET_ACTION                    */
    [3] = {0},                            /* INS_SIGN_ACTION              — BIP32 */
};

const size_t k_command_tlv_configs_count
    = sizeof(k_command_tlv_configs) / sizeof(k_command_tlv_configs[0]);
