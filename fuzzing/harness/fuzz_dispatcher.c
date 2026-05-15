/*
 * Hyperliquid fuzz harness.
 *
 * Generic single-lane dispatcher: one APDU per iteration via
 * fuzz_harness_entry(). All app-specific state lives in the app's globals,
 * which Absolution restores from the input prefix on every iteration; the
 * harness only wires the command table and the dispatcher adapter.
 */

#include "mocks.h"

#include "constants.h"
#include "dispatcher.h"

#include <stdint.h>

#include "scenario_layout.h"

#define FUZZ_PREFIX_SIZE_FALLBACK SCEN_PREFIX_SIZE
#define FUZZ_CTRL_OFF             SCEN_CTRL_OFF
#define FUZZ_CTRL_LEN             SCEN_CTRL_LEN
#define fuzz_lane_is_structured(data, ps) \
    ((ps) > FUZZ_CTRL_OFF && (data)[FUZZ_CTRL_OFF] > FUZZ_STRUCTURED_LANE_THRESHOLD)

#include "fuzz_mutator.h"
#include "fuzz_layout_check.h"
#include "tlv_mutator.h"
#include "fuzz_tlv_config.h"

/* APDU command table.
 *
 * Mirrors apdu_dispatcher() in src/apdu/dispatcher.c:
 *   0x01 GET_ADDRESS             p1=0, p2=0, lc>=1
 *   0x02 PROVIDE_ACTION_METADATA p1=1, p2=0, lc>=2
 *   0x03 SET_ACTION              p1=1, p2=0, lc>=2
 *   0x04 SIGN_ACTION             p1=0, p2=0, lc>=1
 *
 * Use the full p1 byte range and let fuzz_app_dispatch() pin the value
 * expected by the dispatcher: the discriminator checks are part of the
 * coverage surface, but we want the happy path to fire often enough for
 * the deep parsers to be exercised.
 */
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

const size_t fuzz_n_commands = sizeof(fuzz_commands) / sizeof(fuzz_commands[0]);

/* Grammar-aware structural mutator.
 *
 * fuzz_tlv_dispatch_mutate() looks at the active command index (encoded in
 * the prefix's structured-lane control bytes), selects the corresponding
 * TLV grammar from k_command_tlv_configs, and calls tlv_custom_mutate() on
 * the TLV payload region. INS without a TLV body (GET_ADDRESS, SIGN_ACTION)
 * have empty configs and fall through to byte-level mutation.
 *
 * The tlv_custom_mutate() implementation we link against lives in
 * fuzz_tlv_config.c (this app), not in the SDK's fuzzing/mock/tlv_mutator.c.
 * Hyperliquid's lib_tlv parser is DER-encoded; the SDK mutator's single-byte
 * tag emission is malformed for tags >= 0x80, which is almost every content
 * tag in this app. See fuzz_tlv_config.c for the DER-aware replacement.
 */
size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed) {
    return fuzz_tlv_dispatch_mutate(
        data, size, max_size, seed, k_command_tlv_configs, k_command_tlv_configs_count);
}

#include "fuzz_harness.h"

void fuzz_app_reset(void) {
}

void fuzz_app_dispatch(void *cmd) {
    apdu_dispatcher((const command_t *) cmd);
}

int fuzz_entry(const uint8_t *data, size_t size) {
    return fuzz_harness_entry(data, size);
}
