#pragma once

#include <stdbool.h>

#include "ux.h"

#include "io.h"
#include "constants.h"

/**
 * Global structure for NVM data storage.
 */
typedef struct internal_storage_t {
    bool initialized;
} internal_storage_t;

extern const internal_storage_t N_storage_real;
#define N_storage (*(volatile internal_storage_t *) PIC(&N_storage_real))
