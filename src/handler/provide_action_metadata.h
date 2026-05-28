#pragma once

#include <stdbool.h>
#include "buffer.h"

int handler_provide_action_metadata(bool first, const buffer_t *payload);
