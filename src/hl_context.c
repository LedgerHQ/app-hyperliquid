#include <string.h>
#include "hl_context.h"

#define MAX_ACTION_COUNT 10

typedef struct {
    bool has_metadata;
    s_action_metadata metadata;

    s_action actions[MAX_ACTION_COUNT];
    uint8_t action_count;
    uint8_t action_index;
} s_hl_context;

static s_hl_context g_ctx;

void ctx_save_action_metadata(const s_action_metadata *metadata) {
    if (metadata != NULL) {
        g_ctx.has_metadata = true;
        memcpy(&g_ctx.metadata, metadata, sizeof(*metadata));
    }
}

const s_action_metadata *ctx_get_action_metadata(void) {
    if (!g_ctx.has_metadata) {
        return NULL;
    }
    return &g_ctx.metadata;
}

bool ctx_push_action(const s_action *action) {
    if (g_ctx.action_count == MAX_ACTION_COUNT) {
        return false;
    }
    // check that we don't already have an action of the same type stored
    for (int i = 0; i < g_ctx.action_count; ++i) {
        if (action->type == g_ctx.actions[i].type) {
            return false;
        }
    }
    memcpy(&g_ctx.actions[g_ctx.action_count], action, sizeof(*action));
    g_ctx.action_count += 1;
    return true;
}

const s_action *ctx_get_next_action(void) {
    if (g_ctx.action_index == g_ctx.action_count) {
        return NULL;
    }
    g_ctx.action_index += 1;
    return &g_ctx.actions[g_ctx.action_index - 1];
}

bool ctx_is_first_action(void) {
    return g_ctx.action_index == 0;
}

uint8_t ctx_remaining_actions(void) {
    return g_ctx.action_count - g_ctx.action_index;
}

void ctx_reset(void) {
    explicit_bzero(&g_ctx, sizeof(g_ctx));
}
