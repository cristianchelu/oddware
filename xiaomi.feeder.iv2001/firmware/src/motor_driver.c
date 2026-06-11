/*
 * Motor driver — spec/30-processes/dispense-cycle.md § Motor sequencing
 */

#include <stddef.h>

#include "board_gpio_iv2001.h"
#include "motor_driver.h"

#define MOTOR_COAST_STOP_RETRIES  3u

static port_err_t motor_coast_stop(const motor_driver_state_t *state)
{
    if (state == NULL || state->hw.expander == NULL ||
        state->hw.expander->set_pin == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return state->hw.expander->set_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                       BOARD_GPIO_MOTOR_EN_PIN,
                                       false);
}

static port_err_t motor_ph_release(const motor_driver_state_t *state)
{
    if (state == NULL || state->hw.expander == NULL ||
        state->hw.expander->set_pin == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return state->hw.expander->set_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                       BOARD_GPIO_MOTOR_PH_PIN,
                                       false);
}

static port_err_t motor_coast_stop_retry(const motor_driver_state_t *state)
{
    port_err_t err = PORT_ERR_IO;
    uint32_t attempt;

    for (attempt = 0u; attempt < MOTOR_COAST_STOP_RETRIES; attempt++) {
        err = motor_coast_stop(state);
        if (err == PORT_OK) {
            return PORT_OK;
        }
    }

    return err;
}

void motor_driver_init(motor_driver_state_t *state, const motor_hw_t *hw)
{
    if (state == NULL || hw == NULL) {
        return;
    }

    state->hw = *hw;
    state->running = false;
}

static port_err_t motor_driver_run_ph_ms(motor_driver_state_t *state,
                                         bool ph_forward,
                                         uint32_t duration_ms)
{
    port_err_t err;
    bool ph_asserted = false;
    bool en_asserted = false;

    if (state == NULL || state->hw.expander == NULL ||
        state->hw.expander->set_pin == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (duration_ms == 0u || duration_ms > MOTOR_RUN_MS_MAX) {
        return PORT_ERR_INVALID_ARG;
    }

    if (state->running) {
        return PORT_ERR_BUSY;
    }

    state->running = true;

    err = state->hw.expander->set_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                      BOARD_GPIO_MOTOR_PH_PIN,
                                      ph_forward);
    if (err != PORT_OK) {
        goto done;
    }

    ph_asserted = true;

    if (state->hw.delay_ms != NULL) {
        state->hw.delay_ms(MOTOR_PH_SETTLE_MS);
    }

    err = state->hw.expander->set_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                      BOARD_GPIO_MOTOR_EN_PIN,
                                      true);
    if (err != PORT_OK) {
        goto done;
    }

    en_asserted = true;

    if (state->hw.delay_ms != NULL) {
        state->hw.delay_ms(duration_ms);
    }

    err = PORT_OK;

done:
    if (en_asserted) {
        port_err_t stop_err = motor_coast_stop_retry(state);

        if (err == PORT_OK) {
            err = stop_err;
        }
    }

    if (err != PORT_OK && ph_asserted) {
        (void)motor_ph_release(state);
    }

    state->running = false;
    return err;
}

port_err_t motor_driver_run_forward_ms(motor_driver_state_t *state,
                                       uint32_t duration_ms)
{
    return motor_driver_run_ph_ms(state, true, duration_ms);
}

port_err_t motor_driver_run_reverse_ms(motor_driver_state_t *state,
                                       uint32_t duration_ms)
{
    return motor_driver_run_ph_ms(state, false, duration_ms);
}
