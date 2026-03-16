#pragma once

#include "ui_context.h"
#include "action_metadata.h"

bool ui_order(s_ui_ctx *ui_ctx, const s_action_metadata *metadata);
bool ui_modify(s_ui_ctx *ui_ctx, const s_action_metadata *metadata);
bool ui_cancel(s_ui_ctx *ui_ctx, const s_action_metadata *metadata);
bool ui_close(s_ui_ctx *ui_ctx, const s_action_metadata *metadata);
