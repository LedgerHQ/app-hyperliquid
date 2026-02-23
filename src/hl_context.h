#pragma once

#include "action_metadata.h"

void ctx_save_action_metadata(const s_action_metadata *metadata);
const s_action_metadata *ctx_get_action_metadata(void);
void ctx_reset(void);
