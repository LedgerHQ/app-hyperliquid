#include "comm_enabler.h"

static bool g_comm_enabled;

void enable_comm(void) {
    g_comm_enabled = true;
}

void disable_comm(void) {
    g_comm_enabled = false;
}

bool comm_enabled(void) {
    return g_comm_enabled;
}
