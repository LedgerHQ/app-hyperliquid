#include <stdint.h>
#include <stddef.h>
#include "action_metadata.h"
#include "action.h"
#include "hl_context.h"

static int fuzz_action(const uint8_t *data, size_t size) {
    buffer_t buf = {
        .ptr = (uint8_t *) data,
        .size = size,
        .offset = 0,
    };
    uint8_t domain_hash[32];
    uint8_t message_hash[32];

    if (!parse_action(&buf)) {
        return 1;
    }
    return action_hash(ctx_get_next_action(), ctx_get_action_metadata(), domain_hash, message_hash);
}

/* Main fuzzing handler called by libfuzzer */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    s_action_metadata metadata = {
        .op_type = OP_TYPE_ORDER,
        .asset_id = 42,
        .asset_ticker = "FUZZ",
        .network = NETWORK_MAINNET,
        .has_builder_addr = false,
    };

    ctx_reset();
    ctx_save_action_metadata(&metadata);
    // Run the harness
    fuzz_action(data, size);
    return 0;
}
