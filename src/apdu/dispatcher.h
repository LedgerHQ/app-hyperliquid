#pragma once

#include "parser.h"

/**
 * Enumeration with expected INS of APDU commands.
 */
typedef enum {
    NONE = 0x00,
} command_e;

/**
 * Dispatch APDU command received to the right handler.
 *
 * @param[in] cmd
 *   Structured APDU command (CLA, INS, P1, P2, Lc, Command data).
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int apdu_dispatcher(const command_t *cmd);
