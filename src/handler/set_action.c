#include "io.h"
#include "status_words.h"
#include "set_action.h"
#include "action.h"
#include "chunked_payload.h"

int handler_set_action(bool first, const buffer_t *payload) {
    return io_send_sw(process_chunked_payload(first, payload, &parse_action) ? SWO_SUCCESS
                                                                             : SWO_INCORRECT_DATA);
}
