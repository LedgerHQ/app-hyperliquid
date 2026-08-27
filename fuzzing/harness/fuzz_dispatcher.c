/* Hyperliquid fuzz harness: command table, TLV grammars, dispatcher adapter.
 * App state comes from the invariant, so there is no per-iteration setup. */

#include "constants.h"
#include "dispatcher.h"

#include <stdint.h>

/* Own mutator, so fuzz_mutator.h is included directly. */
#define FUZZ_APP_CUSTOM_MUTATOR
#include "fuzz_mutator.h"
#include "tlv_mutator.h"
#include "fuzz_tlv_config.h"

/* Mirrors apdu_dispatcher() in src/apdu/dispatcher.c. p1_max keeps both the
 * accepted and rejected discriminator values in range. */
#define INS_GET_ADDRESS             0x01
#define INS_PROVIDE_ACTION_METADATA 0x02
#define INS_SET_ACTION              0x03
#define INS_SIGN_ACTION             0x04

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = CLA, .ins = INS_GET_ADDRESS,             .p1_max = 1, .flags = FUZZ_CMD_HAS_DATA},
    {.cla = CLA, .ins = INS_PROVIDE_ACTION_METADATA, .p1_max = 1, .flags = FUZZ_CMD_HAS_DATA},
    {.cla = CLA, .ins = INS_SET_ACTION,              .p1_max = 1, .flags = FUZZ_CMD_HAS_DATA},
    {.cla = CLA, .ins = INS_SIGN_ACTION,             .p1_max = 1, .flags = FUZZ_CMD_HAS_DATA},
};

FUZZ_COMMAND_COUNT();

/* Selects the grammar for the active command; commands without a TLV body
 * have empty configs and fall through to byte-level mutation. */
size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed) {
    return fuzz_tlv_dispatch_mutate(
        data, size, max_size, seed, k_command_tlv_configs, k_command_tlv_configs_count);
}

#include "fuzz_harness.h"

void fuzz_app_dispatch(void *cmd) {
    apdu_dispatcher((const command_t *) cmd);
}
