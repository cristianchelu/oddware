/*
 * Motor index IRQ notify — spec/40-architecture/ports.md
 */

#ifndef MOTOR_INDEX_ADAPTER_H
#define MOTOR_INDEX_ADAPTER_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "port_err.h"
#include "task.h"

port_err_t motor_index_adapter_arm_irq(TaskHandle_t task, uint32_t notify_bits);
void motor_index_adapter_disarm_irq(void);

#endif /* MOTOR_INDEX_ADAPTER_H */
