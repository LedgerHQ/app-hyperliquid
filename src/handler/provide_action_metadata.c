#include "io.h"
#include "status_words.h"
#include "provide_action_metadata.h"
#include "action_metadata.h"
#include "chunked_payload.h"

int handler_provide_action_metadata(bool first, const buffer_t *payload) {
    return io_send_sw(process_chunked_payload(first, payload, &parse_action_metadata)
                          ? SWO_SUCCESS
                          : SWO_INCORRECT_DATA);
}
