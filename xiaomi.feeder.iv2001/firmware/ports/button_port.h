/*
 * Button port — spec/40-architecture/ports.md
 *
 * Business-facing tactile button state. Implementations may read AW9523B
 * expander inputs today; hopper IR and other sensors use separate adapters
 * feeding the same process layer — not gpio_expander_port from application code.
 */

#ifndef BUTTON_PORT_H
#define BUTTON_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

typedef struct {
    bool power_pressed;
    bool reset_pressed;
    bool dispense_pressed;
} button_sample_t;

typedef struct button_port {
    port_err_t (*read_sample)(button_sample_t *out);
} button_port_t;

const button_port_t *button_port_get(void);

#endif /* BUTTON_PORT_H */
