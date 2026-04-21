#include "io.h"
#include "status_words.h"
#include "provide_action_metadata.h"
#include "action_metadata.h"

int handler_provide_action_metadata(const buffer_t *payload) {
    return io_send_sw(parse_action_metadata(payload) ? SWO_SUCCESS : SWO_INCORRECT_DATA);
}
