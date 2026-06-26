/*
 * Weight port — spec/40-architecture/ports.md
 */

#ifndef WEIGHT_PORT_H
#define WEIGHT_PORT_H

#include <stdint.h>

#include "port_err.h"
#include "weight_units.h"

typedef enum {
    WEIGHT_CAL_IDLE = 0,
    WEIGHT_CAL_CAPTURING_SPAN,
    WEIGHT_CAL_SUCCESS,
    WEIGHT_CAL_UNCALIBRATED,
} weight_cal_status_t;

typedef struct weight_port {
    port_err_t (*boot_begin)(void);
    port_err_t (*boot_poll)(void);
    port_err_t (*power_on)(void);
    port_err_t (*power_off)(void);
    port_err_t (*read_dg)(weight_dg_t *dg);
    port_err_t (*try_read_dg)(weight_dg_t *dg);
    port_err_t (*read_raw_grams)(int32_t *grams);
    port_err_t (*calibrate_zero)(void);
    port_err_t (*calibrate_span)(void);
    weight_cal_status_t (*get_cal_status)(void);
} weight_port_t;

const weight_port_t *weight_port_get(void);

#endif /* WEIGHT_PORT_H */
