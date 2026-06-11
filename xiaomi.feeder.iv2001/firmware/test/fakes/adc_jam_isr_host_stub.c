#include "adc_jam_isr_adapter.h"

void adc_jam_isr_adapter_start(TaskHandle_t task, uint32_t notify_bits)
{
    (void)task;
    (void)notify_bits;
}

void adc_jam_isr_adapter_stop(void)
{
}
