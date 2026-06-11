/*
 * Host test GPIO expander provider — always the fake AW9523B with WFCI loans.
 */

#include "fake_gpio_expander_port.h"
#include "gpio_expander_port.h"

const gpio_expander_port_t *gpio_expander_port_get(void)
{
    return fake_gpio_expander_port_get();
}
