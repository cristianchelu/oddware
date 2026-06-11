/*
 * ADC mux + sample driver — spec/30-processes/battery-monitoring.md
 */

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>

#include "gpio_expander_port.h"
#include "port_err.h"

typedef port_err_t (*adc_read_raw_fn)(uint16_t *raw);

typedef struct adc_hw {
    const gpio_expander_port_t *expander;
    adc_read_raw_fn read_raw;
    void (*delay_ms)(uint32_t ms);
} adc_hw_t;

typedef struct adc_driver_state {
    adc_hw_t hw;
} adc_driver_state_t;

void adc_driver_init(adc_driver_state_t *state, const adc_hw_t *hw);

port_err_t adc_driver_read_motor_load_mv(adc_driver_state_t *state,
                                         uint16_t *mv);

port_err_t adc_driver_read_battery_mv(adc_driver_state_t *state,
                                      uint16_t *mv);

uint16_t adc_driver_raw_to_mv(uint16_t raw);

#endif /* ADC_DRIVER_H */
