#ifndef FAKE_HOPPER_IR_PORT_H
#define FAKE_HOPPER_IR_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "hopper_ir_port.h"
#include "port_err.h"

void fake_hopper_ir_port_reset(void);
uint32_t fake_hopper_ir_port_sense_count(void);
void fake_hopper_ir_port_set_beam_blocked(bool beam_blocked);
void fake_hopper_ir_port_set_sense_err(port_err_t err);
uint32_t fake_hopper_ir_port_sense_count(void);
const hopper_ir_port_t *fake_hopper_ir_port_get(void);

#endif /* FAKE_HOPPER_IR_PORT_H */
