/*
 * Hopper low-fill broken-beam — spec/30-processes/hopper-sensing.md
 */

#include <stddef.h>

#include "hal_gpio.h"
#include "hal_gpt.h"
#include "hal_pinmux_define.h"

#include "board_gpio_iv2001.h"
#include "gpio_expander_port.h"
#include "hopper_ir_port.h"

#define HOPPER_IR_DRIVE_GPIO  HAL_GPIO_0

static bool s_gpio0_ready;

static void hopper_ir_port_gpio0_init(void)
{
    if (s_gpio0_ready) {
        return;
    }

    hal_gpio_init(HOPPER_IR_DRIVE_GPIO);
    hal_pinmux_set_function(HOPPER_IR_DRIVE_GPIO, HAL_GPIO_0_GPIO0);
    hal_gpio_set_direction(HOPPER_IR_DRIVE_GPIO, HAL_GPIO_DIRECTION_OUTPUT);
    hal_gpio_set_output(HOPPER_IR_DRIVE_GPIO, HAL_GPIO_DATA_LOW);
    s_gpio0_ready = true;
}

static void hopper_ir_port_gpio0_set(bool high)
{
    hopper_ir_port_gpio0_init();
    hal_gpio_set_output(HOPPER_IR_DRIVE_GPIO,
                        high ? HAL_GPIO_DATA_HIGH : HAL_GPIO_DATA_LOW);
}

static bool hopper_ir_port_line_high(uint8_t reg, uint8_t mask)
{
    return (reg & mask) != 0u;
}

static bool hopper_ir_port_beam_blocked(uint8_t p1)
{
    bool high = hopper_ir_port_line_high(p1, BOARD_GPIO_HOPPER_SENSE_MASK);

    if (BOARD_GPIO_HOPPER_BEAM_BLOCKED_HIGH != 0u) {
        return high;
    }

    return !high;
}

static port_err_t hopper_ir_port_sense(bool *beam_blocked)
{
    uint8_t p0;
    uint8_t p1;
    port_err_t err;

    if (beam_blocked == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (gpio_expander_port_get()->read_inputs == NULL) {
        return PORT_ERR_IO;
    }

    hopper_ir_port_gpio0_set(true);
    hal_gpt_delay_ms(BOARD_GPIO_HOPPER_IR_PULSE_MS);

    err = gpio_expander_port_get()->read_inputs(&p0, &p1);
    hopper_ir_port_gpio0_set(false);

    if (err != PORT_OK) {
        return err;
    }

    *beam_blocked = hopper_ir_port_beam_blocked(p1);
    return PORT_OK;
}

static const hopper_ir_port_t s_hopper_ir_port = {
    .sense = hopper_ir_port_sense,
};

const hopper_ir_port_t *hopper_ir_port_get(void)
{
    return &s_hopper_ir_port;
}
