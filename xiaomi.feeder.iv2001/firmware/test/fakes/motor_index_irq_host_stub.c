/*
 * Host stub for motor index IRQ adapter — motor_ctrl tests only.
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "motor_index_adapter.h"
#include "port_err.h"

port_err_t motor_index_adapter_arm_irq(TaskHandle_t task, uint32_t notify_bits)
{
    (void)task;
    (void)notify_bits;
    return PORT_OK;
}

void motor_index_adapter_disarm_irq(void)
{
}
