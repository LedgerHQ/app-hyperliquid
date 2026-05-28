#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "buffer.h"

typedef bool (*f_payload_handler)(const buffer_t *data);

bool process_chunked_payload(bool first, const buffer_t *data, f_payload_handler handler);
