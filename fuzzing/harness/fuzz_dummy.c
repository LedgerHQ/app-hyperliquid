#include <stdint.h>
#include <stddef.h>

int fuzz_dummy(const uint8_t *data, size_t size) {
    (void) data;
    (void) size;
    return 0;
}

/* Main fuzzing handler called by libfuzzer */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // Run the harness
    fuzz_dummy(data, size);

    return 0;
}
