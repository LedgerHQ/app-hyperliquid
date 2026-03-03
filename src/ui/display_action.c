#include "os_print.h"
#include "io.h"
#include "status_words.h"
#include "nbgl_use_case.h"
#include "format.h"
#include "display_action.h"
#include "hl_context.h"
#include "menu.h"
#include "sign_action.h"

#ifdef SCREEN_SIZE_WALLET
#define APP_REVIEW_ICON LARGE_REVIEW_ICON
#else
#define APP_REVIEW_ICON REVIEW_ICON
#endif

#define REVIEW_SIGN_STRING_LENGTH 128
#define MAX_UI_PAIRS              16

static nbgl_contentTagValue_t g_pairs[MAX_UI_PAIRS];
static nbgl_contentTagValueList_t g_pair_list;

static void review_choise(bool confirm) {
    if (confirm) {
        if (sign_action()) {
            nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_SIGNED, ui_menu_main);
        } else {
            ctx_reset();
            ui_menu_main();
        }
    } else {
        ctx_reset();
        io_send_sw(SWO_CONDITIONS_NOT_SATISFIED);
        ui_menu_main();
    }
}

typedef struct {
    char review[REVIEW_SIGN_STRING_LENGTH + 1];
    char sign[REVIEW_SIGN_STRING_LENGTH + 1];
    union {
    };
} s_ui_strings;

static s_ui_strings g_ui_strings;

bool handle_ui(const s_action_metadata *metadata) {
    bool ret;

    if (metadata == NULL) {
        return false;
    }
    explicit_bzero(&g_pair_list, sizeof(g_pair_list));
    explicit_bzero(&g_pairs, sizeof(g_pairs));
    explicit_bzero(&g_ui_strings, sizeof(g_ui_strings));
    g_pair_list.pairs = (nbgl_contentTagValue_t *) &g_pairs;

    g_pairs[g_pair_list.nbPairs].item = "Protocol";
    g_pairs[g_pair_list.nbPairs].value = APPNAME;
    g_pair_list.nbPairs += 1;

    switch (metadata->op_type) {
        default:
            PRINTF("Error: no UI flow for given operation type (%u)!\n", metadata->op_type);
            ret = false;
    }
    if (ret) {
        nbgl_useCaseReview(TYPE_MESSAGE,
                           &g_pair_list,
                           &APP_REVIEW_ICON,
                           g_ui_strings.review,
                           NULL,
                           g_ui_strings.sign,
                           &review_choise);
    }
    return ret;
}
