#pragma once

#include "parser.h"

/**
 * Enumeration with expected INS of APDU commands.
 */
typedef enum {
    GET_ADDRESS = 0x01,
    PROVIDE_ACTION_METADATA = 0x02,
    SET_ACTION = 0x03,
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
