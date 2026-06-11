#ifndef FAKE_MOTOR_INDEX_PORT_H
#define FAKE_MOTOR_INDEX_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "motor_index_port.h"
#include "port_err.h"

void fake_motor_index_port_reset(void);
void fake_motor_index_port_set_led(bool on);
bool fake_motor_index_port_get_led(void);
bool fake_motor_index_port_session_active(void);
void fake_motor_index_port_set_beam_open(bool beam_open);
void fake_motor_index_port_set_read_err(port_err_t err);
void fake_motor_index_port_set_poll_busy_remaining(uint32_t count);
void fake_motor_index_port_set_led_err(port_err_t err);
uint32_t fake_motor_index_port_set_led_calls(void);
const motor_index_port_t *fake_motor_index_port_get(void);

#endif /* FAKE_MOTOR_INDEX_PORT_H */
