#pragma once

#include <stdbool.h>  // bool
#include "action_metadata.h"

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define ICON_APP_BOILERPLATE C_nano_app_hyperliquid
#define ICON_APP_HOME        C_hyperliquid_14px
#define ICON_APP_WARNING     C_icon_warning
#elif defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_BOILERPLATE C_hyperliquid_64px
#define ICON_APP_HOME        ICON_APP_BOILERPLATE
#define ICON_APP_WARNING     C_Warning_64px
#elif defined(TARGET_APEX_P)
#define ICON_APP_BOILERPLATE C_hyperliquid_48px
#define ICON_APP_HOME        ICON_APP_BOILERPLATE
#define ICON_APP_WARNING     LARGE_WARNING_ICON
#endif

bool handle_ui(const s_action_metadata *metadata);
