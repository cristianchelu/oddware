/*
 * AW9523B tactile inputs — spec/30-processes/button-handling.md
 */

#include "board_gpio_iv2001.h"
#include "button_port.h"
#include "gpio_expander_port.h"

static bool button_port_active_low_pressed(uint8_t reg, uint8_t mask)
{
    return (reg & mask) == 0u;
}

static port_err_t button_port_read_sample(button_sample_t *out)
{
    uint8_t p0;
    uint8_t p1;
    port_err_t err;

    if (out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (gpio_expander_port_get()->read_inputs == NULL) {
        return PORT_ERR_IO;
    }

    err = gpio_expander_port_get()->read_inputs(&p0, &p1);
    if (err != PORT_OK) {
        return err;
    }

    out->power_pressed =
        button_port_active_low_pressed(p0, BOARD_GPIO_BTN_POWER_MASK);
    out->reset_pressed =
        button_port_active_low_pressed(p0, BOARD_GPIO_BTN_RESET_MASK);
    out->dispense_pressed =
        button_port_active_low_pressed(p1, BOARD_GPIO_BTN_DISPENSE_MASK);
    return PORT_OK;
}

static const button_port_t s_button_port = {
    .read_sample = button_port_read_sample,
};

const button_port_t *button_port_get(void)
{
    return &s_button_port;
}
