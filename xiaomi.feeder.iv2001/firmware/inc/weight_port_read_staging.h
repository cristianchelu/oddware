/*
 * Staged read results for weight port adapter UART loan callbacks.
 * spec/40-architecture/ports.md — read_dg and read_raw_grams are separate paths.
 */

#ifndef WEIGHT_PORT_READ_STAGING_H
#define WEIGHT_PORT_READ_STAGING_H

#include <stdint.h>

#include "weight_units.h"

typedef struct {
    weight_dg_t dg;
    int32_t raw_grams;
} weight_port_read_staging_t;

#endif /* WEIGHT_PORT_READ_STAGING_H */
