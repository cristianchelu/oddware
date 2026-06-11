#ifndef FAKE_GPIO_EXPANDER_PORT_H
#define FAKE_GPIO_EXPANDER_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "gpio_expander_port.h"

void fake_gpio_expander_reset(void);
void fake_gpio_expander_set_set_pin_fail_on(uint32_t nth, port_err_t err);
void fake_gpio_expander_set_set_pin_fail_range(uint32_t from,
                                             uint32_t until,
                                             port_err_t err);
bool fake_gpio_expander_get_set_pin_log(uint32_t index,
                                        uint8_t *port,
                                        uint8_t *pin,
                                        bool *level);
uint32_t fake_gpio_expander_set_pin_calls(void);
void fake_gpio_expander_set_reset_err(port_err_t err);
void fake_gpio_expander_set_id(uint8_t id);
void fake_gpio_expander_set_inputs(uint8_t p0, uint8_t p1);
void fake_gpio_expander_set_index_beam_open(bool open);
uint8_t fake_gpio_expander_out_p0(void);
bool fake_gpio_expander_pin(uint8_t port, uint8_t pin);
const gpio_expander_port_t *fake_gpio_expander_port_get(void);

#endif /* FAKE_GPIO_EXPANDER_PORT_H */
