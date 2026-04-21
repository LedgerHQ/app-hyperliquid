/* bytes_to_lowercase_hex — pure utility function used by bulk_order.c.
 * Copied verbatim from $BOLOS_SDK/src/os.c; no hardware dependencies.
 */
#include <stddef.h>
#include <stdint.h>

int bytes_to_lowercase_hex(char *out, size_t outl, const void *value, size_t len) {
    const uint8_t *bytes = (const uint8_t *) value;
    const char    *hex   = "0123456789abcdef";

    if (outl < 2 * len + 1) {
        *out = '\0';
        return -1;
    }
    for (size_t i = 0; i < len; i++) {
        *out++ = hex[(bytes[i] >> 4) & 0xf];
        *out++ = hex[bytes[i] & 0xf];
    }
    *out = '\0';
    return 0;
}

/* pic/pic_init — hardware PIC relocation stubs.
 * On host, PIC(x) expands to pic((void*)x). Simply return the pointer unchanged.
 */
void *pic(void *linked_address) {
    return linked_address;
}

void pic_init(void *pic_flash_start, void *pic_ram_start) {
    (void) pic_flash_start;
    (void) pic_ram_start;
}
