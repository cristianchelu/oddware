#ifndef ADC_BUS_ADAPTER_H
#define ADC_BUS_ADAPTER_H

#include <stdint.h>

#include "port_err.h"

void adc_bus_adapter_init(void);
void adc_bus_adapter_deinit(void);
port_err_t adc_bus_adapter_read_raw(uint16_t *raw);

#endif /* ADC_BUS_ADAPTER_H */
