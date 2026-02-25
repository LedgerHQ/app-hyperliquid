#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "action_metadata.h"
#include "action.h"

void ctx_save_action_metadata(const s_action_metadata *metadata);
const s_action_metadata *ctx_get_action_metadata(void);

bool ctx_push_action(const s_action *action);
const s_action *ctx_get_next_action(void);
bool ctx_is_first_action(void);
uint8_t ctx_remaining_actions(void);

void ctx_reset(void);
