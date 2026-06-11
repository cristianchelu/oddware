/*
 * Motor control task — spec/40-architecture/task-model.md,
 * spec/30-processes/dispense-cycle.md
 */

#ifndef MOTOR_CTRL_H
#define MOTOR_CTRL_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

#define MOTOR_CTRL_NOTIFY_INDEX   (1u << 0)
#define MOTOR_CTRL_NOTIFY_ADC_JAM (1u << 1)

/* Owned constants — spec/40-architecture/task-model.md (SDK TASK_PRIORITY_HIGH = 6). */
#define MOTOR_CTRL_TASK_NAME         "motor_ctrl"
#define MOTOR_CTRL_TASK_STACK_BYTES  2048u
#define MOTOR_CTRL_TASK_PRIO         6u

void motor_ctrl_start(void);
bool motor_ctrl_is_active(void);

port_err_t motor_ctrl_request_burst(uint8_t pulse_target, uint16_t timeout_ms);
port_err_t motor_ctrl_request_park(uint8_t max_pulses);
port_err_t motor_ctrl_request_timed_forward_ms(uint32_t duration_ms);
port_err_t motor_ctrl_request_timed_reverse_ms(uint32_t duration_ms);
port_err_t motor_ctrl_request_stop(void);

#endif /* MOTOR_CTRL_H */
