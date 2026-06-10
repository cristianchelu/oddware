/*
 * GPIO4 EINT for AW9523B INT — spec/30-processes/button-handling.md § IRQ dispatch
 */

#ifndef AW9523_IRQ_ADAPTER_H
#define AW9523_IRQ_ADAPTER_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "port_err.h"
#include "task.h"

port_err_t aw9523_irq_adapter_start(void);

void aw9523_irq_adapter_register_motor_notify(TaskHandle_t task, uint32_t notify_bits);
void aw9523_irq_adapter_unregister_motor_notify(void);

#endif /* AW9523_IRQ_ADAPTER_H */
