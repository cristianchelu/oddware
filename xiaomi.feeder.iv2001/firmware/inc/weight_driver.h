/*
 * Weight driver composition — spec/30-processes/weighing.md
 */

#ifndef WEIGHT_DRIVER_H
#define WEIGHT_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "config_port.h"
#include "cs1270.h"
#include "weigh_cal.h"
#include "gpio_expander_port.h"
#include "port_err.h"
#include "weight_port.h"
#include "weight_rail.h"

typedef struct weight_hw {
    const gpio_expander_port_t *expander;
    const cs1270_uart_ops_t *uart;
    const config_port_t *config;
    void (*delay_ms)(uint32_t ms);
} weight_hw_t;

typedef struct weight_driver_state {
    weight_hw_t hw;
    weight_rail_ctx_t rail;
    bool powered;
    bool boot_done;
    bool scale_off;
    weigh_cal_model_t cal;
} weight_driver_state_t;

void weight_driver_init(weight_driver_state_t *state, const weight_hw_t *hw);

/* Rail assert runs under a short EXPANDER loan; boot settle must run with no WFCI loan. */
port_err_t weight_rail_enable(weight_driver_state_t *state);
port_err_t weight_boot_settle(weight_driver_state_t *state);
port_err_t weight_power_off(weight_driver_state_t *state);
bool weight_scale_off(const weight_driver_state_t *state);
port_err_t weight_read_dg(weight_driver_state_t *state, weight_dg_t *dg);
port_err_t weight_read_raw_grams(weight_driver_state_t *state, int32_t *grams);
port_err_t weight_calibrate_zero(weight_driver_state_t *state);
port_err_t weight_calibrate_span(weight_driver_state_t *state);
weight_cal_status_t weight_driver_cal_status(const weight_driver_state_t *state);

#endif /* WEIGHT_DRIVER_H */
