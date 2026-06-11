/*
 * Motor index broken-beam — spec/30-processes/motor-index.md
 */

#include <stddef.h>

#include "board_gpio_iv2001.h"
#include "gpio_expander_port.h"
#include "motor_index_port.h"

static bool s_session_active;

static bool motor_index_port_line_high(uint8_t reg, uint8_t mask)
{
    return (reg & mask) != 0u;
}

static bool motor_index_port_beam_open(uint8_t p0)
{
    bool high = motor_index_port_line_high(p0, BOARD_GPIO_INDEX_DET_MASK);

    if (BOARD_GPIO_INDEX_BEAM_OPEN_HIGH != 0u) {
        return high;
    }

    return !high;
}

static port_err_t motor_index_port_set_led(bool on)
{
    const gpio_expander_port_t *exp = gpio_expander_port_get();

    if (exp == NULL || exp->set_pin == NULL) {
        return PORT_ERR_IO;
    }

    return exp->set_pin(BOARD_GPIO_INDEX_LED_PORT,
                        BOARD_GPIO_INDEX_LED_PIN,
                        on);
}

static port_err_t motor_index_port_read_detector(bool *beam_open, bool try_only)
{
    const gpio_expander_port_t *exp = gpio_expander_port_get();
    uint8_t p0;
    uint8_t p1;
    port_err_t err;

    if (beam_open == NULL || exp == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (try_only) {
        if (exp->try_read_inputs == NULL) {
            return PORT_ERR_IO;
        }
        err = exp->try_read_inputs(&p0, &p1);
    } else {
        if (exp->read_inputs == NULL) {
            return PORT_ERR_IO;
        }
        err = exp->read_inputs(&p0, &p1);
    }

    if (err != PORT_OK) {
        return err;
    }

    *beam_open = motor_index_port_beam_open(p0);
    return PORT_OK;
}

static port_err_t motor_index_port_sense(bool *beam_open)
{
    bool restore_session = s_session_active;
    port_err_t err;
    port_err_t off_err;
    port_err_t restore_err = PORT_OK;

    err = motor_index_port_set_led(true);
    if (err != PORT_OK) {
        return err;
    }

    err = motor_index_port_read_detector(beam_open, false);
    off_err = motor_index_port_set_led(false);
    if (restore_session) {
        restore_err = motor_index_port_set_led(true);
    }

    if (err != PORT_OK) {
        return err;
    }

    if (off_err != PORT_OK) {
        return off_err;
    }

    return restore_err;
}

static port_err_t motor_index_port_session_begin(void)
{
    port_err_t err;

    if (s_session_active) {
        return PORT_OK;
    }

    err = motor_index_port_set_led(true);
    if (err == PORT_OK) {
        s_session_active = true;
    }

    return err;
}

static port_err_t motor_index_port_session_end(void)
{
    port_err_t err;

    if (!s_session_active) {
        return PORT_OK;
    }

    s_session_active = false;
    err = motor_index_port_set_led(false);
    return err;
}

static port_err_t motor_index_port_poll(bool *beam_open)
{
    if (!s_session_active) {
        return PORT_ERR_IO;
    }

    return motor_index_port_read_detector(beam_open, true);
}

static const motor_index_port_t s_motor_index_port = {
    .sense = motor_index_port_sense,
    .session_begin = motor_index_port_session_begin,
    .session_end = motor_index_port_session_end,
    .poll = motor_index_port_poll,
};

const motor_index_port_t *motor_index_port_adapter_get(void)
{
    return &s_motor_index_port;
}

#ifndef HOST_TEST
const motor_index_port_t *motor_index_port_get(void)
{
    return motor_index_port_adapter_get();
}
#endif
