/*
 * Motor index IRQ notify registration — spec/40-architecture/ports.md
 */

#include "aw9523_irq_adapter.h"
#include "motor_index_adapter.h"

port_err_t motor_index_adapter_arm_irq(TaskHandle_t task, uint32_t notify_bits)
{
    if (task == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    aw9523_irq_adapter_register_motor_notify(task, notify_bits);
    return PORT_OK;
}

void motor_index_adapter_disarm_irq(void)
{
    aw9523_irq_adapter_unregister_motor_notify();
}
