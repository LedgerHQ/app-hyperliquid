#include <string.h>
#include "os_print.h"
#include "read.h"
#include "chunked_payload.h"

#define PAYLOAD_BUFFER_SIZE (0xff * 4)

typedef struct {
    uint8_t buffer[PAYLOAD_BUFFER_SIZE];
    uint16_t size;
    uint16_t received;
    f_payload_handler handler;
} s_chunked_payload;

static s_chunked_payload g_payload;

static bool process_chunked_payload_internal(bool first,
                                             const buffer_t *data,
                                             f_payload_handler handler,
                                             s_chunked_payload *payload) {
    size_t offset;

    if (data == NULL) {
        PRINTF("Error: NULL data!\n");
        return false;
    }

    if (first) {
        if (payload->handler != NULL) {
            PRINTF("Error: new first chunk while session is in progress!\n");
            return false;
        }
        if (data->size < sizeof(payload->size)) {
            PRINTF("Error: length too small to have a size!\n");
            return false;
        }
        payload->size = read_u16_be(data->ptr, 0);
        if ((payload->size == 0) || (payload->size > sizeof(payload->buffer))) {
            PRINTF("Error: cannot process payload of such size (%u)!\n", payload->size);
            return false;
        }
        payload->received = 0;
        offset = sizeof(payload->size);
        payload->handler = handler;
    } else {
        if (data->size == 0) {
            PRINTF("Error: empty following chunk!\n");
            return false;
        }
        if (handler != payload->handler) {
            PRINTF(
                "Error: following chunk for a different handler than the already buffered one!\n");
            return false;
        }
        offset = 0;
    }

    if ((payload->received + data->size - offset) > payload->size) {
        PRINTF("Error: chunk too big for payload size!\n");
        return false;
    }

    memcpy(&payload->buffer[payload->received], &data->ptr[offset], data->size - offset);
    payload->received += (uint16_t) (data->size - offset);

    if (payload->received == payload->size) {
        buffer_t buf = {
            .ptr = payload->buffer,
            .size = payload->size,
        };
        if (!handler(&buf)) {
            return false;
        }
        explicit_bzero(payload, sizeof(*payload));
    }
    return true;
}

bool process_chunked_payload(bool first, const buffer_t *data, f_payload_handler handler) {
    if (!process_chunked_payload_internal(first, data, handler, &g_payload)) {
        explicit_bzero(&g_payload, sizeof(g_payload));
        return false;
    }
    return true;
}
