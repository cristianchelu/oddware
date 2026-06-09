#ifndef FAKE_GPIO_EXPANDER_PORT_H
#define FAKE_GPIO_EXPANDER_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "gpio_expander_port.h"

void fake_gpio_expander_reset(void);
void fake_gpio_expander_set_reset_err(port_err_t err);
void fake_gpio_expander_set_id(uint8_t id);
uint8_t fake_gpio_expander_out_p0(void);
bool fake_gpio_expander_pin(uint8_t port, uint8_t pin);
const gpio_expander_port_t *fake_gpio_expander_port_get(void);

#endif /* FAKE_GPIO_EXPANDER_PORT_H */
