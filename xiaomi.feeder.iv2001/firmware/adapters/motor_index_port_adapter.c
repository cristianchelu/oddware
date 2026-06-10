/*
 * Motor index broken-beam — spec/30-processes/motor-index.md
 */

#include <stddef.h>

#include "board_gpio_iv2001.h"
#include "gpio_expander_port.h"
#include "motor_index_port.h"

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

    if (exp->set_pin == NULL) {
        return PORT_ERR_IO;
    }

    return exp->set_pin(BOARD_GPIO_INDEX_LED_PORT,
                        BOARD_GPIO_INDEX_LED_PIN,
                        on);
}

static port_err_t motor_index_port_read_beam_open(bool *beam_open)
{
    uint8_t p0;
    uint8_t p1;
    port_err_t err;

    if (beam_open == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (gpio_expander_port_get()->read_inputs == NULL) {
        return PORT_ERR_IO;
    }

    err = gpio_expander_port_get()->read_inputs(&p0, &p1);
    if (err != PORT_OK) {
        return err;
    }

    *beam_open = motor_index_port_beam_open(p0);
    return PORT_OK;
}

static const motor_index_port_t s_motor_index_port = {
    .set_led = motor_index_port_set_led,
    .read_beam_open = motor_index_port_read_beam_open,
};

const motor_index_port_t *motor_index_port_get(void)
{
    return &s_motor_index_port;
}
