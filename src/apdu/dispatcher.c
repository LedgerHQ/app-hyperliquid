/*****************************************************************************
 *   Ledger App Boilerplate.
 *   (c) 2020 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include "buffer.h"
#include "io.h"
#include "ledger_assert.h"
#include "status_words.h"

#include "dispatcher.h"
#include "constants.h"
#include "globals.h"

#include "get_addr.h"

#define P1_FIRST     0x01
#define P1_FOLLOWING 0x00

int apdu_dispatcher(const command_t *cmd) {
    LEDGER_ASSERT(cmd != NULL, "NULL cmd");

    if (cmd->cla != CLA) {
        return io_send_sw(SWO_INVALID_CLA);
    }

    buffer_t buf = {0};

    switch (cmd->ins) {
        case GET_ADDRESS:
            if ((cmd->p1 != 0) || (cmd->p2 != 0)) {
                return io_send_sw(SWO_INCORRECT_P1_P2);
            }
            if (!cmd->data || (cmd->lc < 1)) {
                return io_send_sw(SWO_WRONG_DATA_LENGTH);
            }
            buf.size = cmd->lc;
            buf.ptr = cmd->data;
            return handler_get_addr(&buf);

        default:
            return io_send_sw(SWO_INVALID_INS);
    }
}
