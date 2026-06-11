/*
 * Motor port — spec/40-architecture/ports.md
 */

#ifndef MOTOR_PORT_H
#define MOTOR_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

typedef struct motor_port {
    port_err_t (*run_forward_ms)(uint32_t duration_ms);
    port_err_t (*run_reverse_ms)(uint32_t duration_ms);
    port_err_t (*request_burst)(uint8_t pulse_target, uint16_t timeout_ms);
    port_err_t (*request_park)(uint8_t max_pulses);
    port_err_t (*stop)(void);
    bool (*is_active)(void);
} motor_port_t;

const motor_port_t *motor_port_get(void);

#endif /* MOTOR_PORT_H */
