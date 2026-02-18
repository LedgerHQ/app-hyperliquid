#include "io.h"
#include "status_words.h"
#include "set_action.h"
#include "action.h"

int handler_set_action(const buffer_t *payload) {
    return io_send_sw(parse_action(payload) ? SWO_SUCCESS : SWO_INCORRECT_DATA);
}
