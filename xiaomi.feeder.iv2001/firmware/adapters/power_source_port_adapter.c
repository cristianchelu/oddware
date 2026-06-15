/*
 * Mains / barrel jack presence — spec/30-processes/power-state-machine.md
 */

#include <stddef.h>

#include "board_gpio_iv2001.h"
#include "gpio_expander_port.h"
#include "power_source_port.h"

static bool power_source_port_line_high(uint8_t reg, uint8_t mask)
{
    return (reg & mask) != 0u;
}

static bool power_source_port_mains_present(uint8_t p1)
{
    bool high = power_source_port_line_high(p1, BOARD_GPIO_MAINS_SENSE_MASK);

    if (BOARD_GPIO_MAINS_PRESENT_HIGH != 0u) {
        return high;
    }

    return !high;
}

static port_err_t power_source_port_read_present(bool *mains_present)
{
    const gpio_expander_port_t *exp = gpio_expander_port_get();
    uint8_t p0;
    uint8_t p1;
    port_err_t err;

    if (mains_present == NULL || exp == NULL || exp->read_inputs == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = exp->read_inputs(&p0, &p1);
    if (err != PORT_OK) {
        return err;
    }

    *mains_present = power_source_port_mains_present(p1);
    return PORT_OK;
}

static const power_source_port_t s_power_source_port = {
    .read_present = power_source_port_read_present,
};

const power_source_port_t *power_source_port_adapter_get(void)
{
    return &s_power_source_port;
}

#ifndef HOST_TEST
const power_source_port_t *power_source_port_get(void)
{
    return power_source_port_adapter_get();
}
#endif
