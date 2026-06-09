/*
 * AW9523B bootstrap sequence — spec/30-processes/display-driver.md
 */

#ifndef GPIO_EXPANDER_BOOTSTRAP_H
#define GPIO_EXPANDER_BOOTSTRAP_H

#include "gpio_expander_port.h"
#include "port_err.h"

port_err_t gpio_expander_bootstrap(const gpio_expander_port_t *exp);

#endif /* GPIO_EXPANDER_BOOTSTRAP_H */
