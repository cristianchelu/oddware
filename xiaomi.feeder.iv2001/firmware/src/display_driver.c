/*
 * Display driver composition — spec/30-processes/display-driver.md
 */

#include <stddef.h>

#include "display_driver.h"
#include "gpio_expander_bootstrap.h"

static port_err_t display_refresh(display_driver_state_t *state, uint8_t segment_byte)
{
    uint8_t grids[TM1637_GRID_COUNT];

    if (!display_rail_is_settled(&state->rail)) {
        return PORT_ERR_BUSY;
    }

    for (uint8_t i = 0u; i < TM1637_GRID_COUNT; i++) {
        grids[i] = segment_byte;
    }

    return tm1637_refresh(state->hw.tm1637_gpio, grids, TM1637_BRIGHTNESS_MAX);
}

void display_driver_init(display_driver_state_t *state, const display_hw_t *hw)
{
    state->hw = *hw;
    display_rail_ctx_init(&state->rail, hw->expander, hw->delay_ms);
    state->powered = false;
}

port_err_t display_power_on(display_driver_state_t *state)
{
    port_err_t err;

    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = gpio_expander_bootstrap(state->hw.expander);
    if (err != PORT_OK) {
        return err;
    }

    if (state->hw.prepare_tm1637_pins != NULL) {
        state->hw.prepare_tm1637_pins();
    }

    err = display_rail_on(&state->rail);
    if (err != PORT_OK) {
        return err;
    }

    err = display_refresh(state, 0x00u);
    if (err != PORT_OK) {
        return err;
    }

    state->powered = true;
    return PORT_OK;
}

port_err_t display_power_off(display_driver_state_t *state)
{
    port_err_t err;

    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = display_rail_off(&state->rail);
    if (err == PORT_OK) {
        state->powered = false;
    }
    return err;
}

port_err_t display_show_fill(display_driver_state_t *state, uint8_t segment_byte)
{
    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }
    if (!state->powered) {
        return PORT_ERR_BUSY;
    }

    return display_refresh(state, segment_byte);
}

port_err_t display_show_grids(display_driver_state_t *state,
                              const uint8_t grids[TM1637_GRID_COUNT])
{
    if (state == NULL || grids == NULL) {
        return PORT_ERR_INVALID_ARG;
    }
    if (!state->powered) {
        return PORT_ERR_BUSY;
    }
    if (!display_rail_is_settled(&state->rail)) {
        return PORT_ERR_BUSY;
    }

    return tm1637_refresh(state->hw.tm1637_gpio, grids, TM1637_BRIGHTNESS_MAX);
}

port_err_t display_blank(display_driver_state_t *state)
{
    return display_show_fill(state, 0x00u);
}
