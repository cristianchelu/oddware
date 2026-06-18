#ifndef FAKE_MOTOR_PORT_H
#define FAKE_MOTOR_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "motor_port.h"
#include "port_err.h"

void fake_motor_port_reset(void);
void fake_motor_port_set_timed_fwd_err(port_err_t err);
void fake_motor_port_set_timed_rev_err(port_err_t err);
void fake_motor_port_set_burst_err(port_err_t err);
void fake_motor_port_set_active(bool active);
void fake_motor_port_set_defer_burst_active(bool defer);
void fake_motor_port_complete_burst(void);
uint32_t fake_motor_port_timed_fwd_calls(void);
uint32_t fake_motor_port_last_timed_fwd_ms(void);
uint32_t fake_motor_port_timed_rev_calls(void);
uint32_t fake_motor_port_last_timed_rev_ms(void);
uint32_t fake_motor_port_burst_calls(void);
uint8_t fake_motor_port_last_pulse_target(void);
uint16_t fake_motor_port_last_timeout_ms(void);
uint32_t fake_motor_port_park_calls(void);
const motor_port_t *fake_motor_port_get(void);

#endif /* FAKE_MOTOR_PORT_H */
