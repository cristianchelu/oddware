/*
 * Hopper low-fill broken-beam port — spec/40-architecture/ports.md
 */

#ifndef HOPPER_IR_PORT_H
#define HOPPER_IR_PORT_H

#include <stdbool.h>

#include "port_err.h"

typedef struct hopper_ir_port {
    port_err_t (*sense)(bool *beam_blocked);
} hopper_ir_port_t;

const hopper_ir_port_t *hopper_ir_port_get(void);

#endif /* HOPPER_IR_PORT_H */
