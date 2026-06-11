/*
 * GPIO expander port — spec/40-architecture/ports.md
 */

#ifndef GPIO_EXPANDER_PORT_H
#define GPIO_EXPANDER_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

typedef struct gpio_expander_port {
    port_err_t (*reset)(void);
    port_err_t (*configure)(uint8_t dir_p0,
                            uint8_t dir_p1,
                            uint8_t out_p0,
                            uint8_t out_p1);
    port_err_t (*set_pin)(uint8_t port, uint8_t pin, bool level);
    port_err_t (*get_pin)(uint8_t port, uint8_t pin, bool *level);
    port_err_t (*read_inputs)(uint8_t *p0, uint8_t *p1);
    port_err_t (*try_read_inputs)(uint8_t *p0, uint8_t *p1);
    port_err_t (*set_int_mask)(uint8_t mask_p0, uint8_t mask_p1);
} gpio_expander_port_t;

const gpio_expander_port_t *gpio_expander_port_get(void);

#endif /* GPIO_EXPANDER_PORT_H */
