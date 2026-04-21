#include <stdint.h>
#include <stddef.h>
#include "action_metadata.h"
#include "hl_context.h"

static int fuzz_action_metadata(const uint8_t *data, size_t size) {
    buffer_t buf = {
        .ptr = (uint8_t *) data,
        .size = size,
        .offset = 0,
    };

    return parse_action_metadata(&buf);
}

/* Main fuzzing handler called by libfuzzer */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    ctx_reset();
    // Run the harness
    fuzz_action_metadata(data, size);
    return 0;
}
