/*
 * Motor driver — spec/30-processes/dispense-cycle.md § Motor sequencing
 */

#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#include "gpio_expander_port.h"
#include "motor_limits.h"
#include "port_err.h"

typedef struct motor_hw {
    const gpio_expander_port_t *expander;
    void (*delay_ms)(uint32_t ms);
} motor_hw_t;

typedef struct motor_driver_state {
    motor_hw_t hw;
    bool running;
} motor_driver_state_t;

void motor_driver_init(motor_driver_state_t *state, const motor_hw_t *hw);
port_err_t motor_driver_start_forward(motor_driver_state_t *state);
port_err_t motor_driver_start_reverse(motor_driver_state_t *state);
port_err_t motor_driver_stop(motor_driver_state_t *state);
bool motor_driver_is_running(const motor_driver_state_t *state);
port_err_t motor_driver_run_forward_ms(motor_driver_state_t *state,
                                       uint32_t duration_ms);
port_err_t motor_driver_run_reverse_ms(motor_driver_state_t *state,
                                       uint32_t duration_ms);

#endif /* MOTOR_DRIVER_H */
