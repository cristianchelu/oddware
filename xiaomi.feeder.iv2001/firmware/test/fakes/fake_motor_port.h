#ifndef FAKE_MOTOR_PORT_H
#define FAKE_MOTOR_PORT_H

#include <stdint.h>

#include "motor_port.h"
#include "port_err.h"

void fake_motor_port_reset(void);
void fake_motor_port_set_run_err(port_err_t err);
void fake_motor_port_set_reverse_err(port_err_t err);
uint32_t fake_motor_port_run_calls(void);
uint32_t fake_motor_port_last_duration_ms(void);
uint32_t fake_motor_port_reverse_calls(void);
uint32_t fake_motor_port_last_reverse_duration_ms(void);
const motor_port_t *fake_motor_port_get(void);

#endif /* FAKE_MOTOR_PORT_H */
