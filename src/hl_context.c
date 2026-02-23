#include <stdbool.h>
#include <string.h>
#include "hl_context.h"

typedef struct {
    bool has_metadata;
    s_action_metadata metadata;
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

void ctx_reset(void) {
    explicit_bzero(&g_ctx, sizeof(g_ctx));
}
