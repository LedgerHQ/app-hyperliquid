#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "ui_context.h"
#include "action_metadata.h"
#include "order_request.h"

void show_metadata_margin(s_ui_ctx *ui_ctx,
                          const s_action_metadata *metadata,
                          char *out,
                          size_t size);
void show_metadata_leverage(s_ui_ctx *ui_ctx,
                            const s_action_metadata *metadata,
                            char *out,
                            size_t size);
typedef bool (*f_order_request_matcher)(const s_order_request *);

const s_order_request *get_order_request(const s_order_request *list,
                                         size_t size,
                                         f_order_request_matcher match_func);
bool get_limit_request(const s_order_request *req);
bool get_trigger_tp(const s_order_request *req);
bool get_trigger_sl(const s_order_request *req);

const char *get_short_long_string(const s_order_request *req);
const char *get_short_long_string_capitalized(const s_order_request *req);
