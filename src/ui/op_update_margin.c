#include <stdbool.h>
#include <stdio.h>
#include "format.h"
#include "ui_context.h"
#include "hl_context.h"
#include "ui_common.h"

bool ui_update_margin(s_ui_ctx *ui_ctx, const s_action_metadata *metadata) {
    const s_action *action;
    const s_update_isolated_margin *update_isolated_margin;
    // bigger value than necessary until the SDK function is fixed
    char tmp[NUMERIC_STRING_LENGTH + MARGIN_DECIMALS + 1] = {0};
    uint64_t margin;

    if ((action = ctx_get_action(ACTION_TYPE_UPDATE_ISOLATED_MARGIN)) == NULL) {
        return false;
    }
    update_isolated_margin = &action->update_isolated_margin;
    if (update_isolated_margin->ntli < 0) {
        margin = -(uint64_t) update_isolated_margin->ntli;
    } else {
        margin = (uint64_t) update_isolated_margin->ntli;
    }

    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "Active Position";
    snprintf(ui_ctx->update_margin.operation,
             sizeof(ui_ctx->update_margin.operation),
             "%s - %s",
             update_isolated_margin->is_buy ? "Long" : "Short",
             metadata->asset_ticker);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->update_margin.operation;
    ui_ctx->pair_list.nbPairs += 1;

    format_fpu64_trimmed(tmp, sizeof(tmp), margin, MARGIN_DECIMALS);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].item = "New Margin";
    snprintf(ui_ctx->update_margin.margin,
             sizeof(ui_ctx->update_margin.margin),
             "%s %s",
             tmp,
             COUNTERVALUE_TICKER);
    ui_ctx->pairs[ui_ctx->pair_list.nbPairs].value = ui_ctx->update_margin.margin;
    ui_ctx->pair_list.nbPairs += 1;

    show_metadata_leverage(ui_ctx,
                           metadata,
                           ui_ctx->update_margin.leverage,
                           sizeof(ui_ctx->update_margin.leverage));

    snprintf(ui_ctx->intent,
             sizeof(ui_ctx->intent),
#ifdef SCREEN_SIZE_WALLET
             "%s Margin %s %s %s",
             (update_isolated_margin->ntli < 0) ? "Remove" : "Add",
             (update_isolated_margin->ntli < 0) ? "from" : "to",
             metadata->asset_ticker,
             update_isolated_margin->is_buy ? "Long" : "Short"
#else
             "%s margin %s %s",
             (update_isolated_margin->ntli < 0) ? "remove" : "add",
             (update_isolated_margin->ntli < 0) ? "from" : "to",
             update_isolated_margin->is_buy ? "long" : "short"
#endif
    );
    return true;
}
