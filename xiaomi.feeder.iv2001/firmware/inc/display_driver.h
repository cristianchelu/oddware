/*
 * Display driver — spec/30-processes/display-driver.md
 */

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>

#include "display_rail.h"
#include "gpio_expander_port.h"
#include "port_err.h"
#include "tm1637.h"

typedef struct display_hw {
    const gpio_expander_port_t *expander;
    const tm1637_gpio_ops_t *tm1637_gpio;
    void (*prepare_tm1637_pins)(void);
    void (*delay_ms)(uint32_t ms);
} display_hw_t;

typedef struct display_driver_state {
    display_hw_t hw;
    display_rail_ctx_t rail;
    bool powered;
    uint8_t brightness_cmd;
} display_driver_state_t;

void display_driver_init(display_driver_state_t *state, const display_hw_t *hw);

port_err_t display_power_on(display_driver_state_t *state);
port_err_t display_power_off(display_driver_state_t *state);
port_err_t display_show_fill(display_driver_state_t *state, uint8_t segment_byte);
port_err_t display_show_grids(display_driver_state_t *state,
                              const uint8_t grids[TM1637_GRID_COUNT]);
port_err_t display_blank(display_driver_state_t *state);
port_err_t display_set_brightness(display_driver_state_t *state, uint8_t level);

#endif /* DISPLAY_DRIVER_H */
