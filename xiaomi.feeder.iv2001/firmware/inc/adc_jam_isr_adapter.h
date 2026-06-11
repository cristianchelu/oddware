/*
 * GPIO17 motor-load jam ISR — spec/30-processes/jam-detection.md
 */

#ifndef ADC_JAM_ISR_ADAPTER_H
#define ADC_JAM_ISR_ADAPTER_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

void adc_jam_isr_adapter_start(TaskHandle_t task, uint32_t notify_bits);
void adc_jam_isr_adapter_stop(void);

#endif /* ADC_JAM_ISR_ADAPTER_H */
