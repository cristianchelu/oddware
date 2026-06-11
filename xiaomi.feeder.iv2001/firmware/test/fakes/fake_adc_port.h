#ifndef FAKE_ADC_PORT_H
#define FAKE_ADC_PORT_H

#include <stdint.h>

#include "adc_port.h"
#include "port_err.h"

void fake_adc_port_reset(void);

void fake_adc_port_set_motor_mv(uint16_t mv);

void fake_adc_port_set_battery_mv(uint16_t mv);

void fake_adc_port_set_motor_err(port_err_t err);

void fake_adc_port_set_battery_err(port_err_t err);

const adc_port_t *fake_adc_port_get(void);

#endif /* FAKE_ADC_PORT_H */
