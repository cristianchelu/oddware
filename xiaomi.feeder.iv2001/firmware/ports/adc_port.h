/*
 * ADC sense port — spec/40-architecture/ports.md
 */

#ifndef ADC_PORT_H
#define ADC_PORT_H

#include <stdint.h>

#include "port_err.h"

typedef struct adc_port {
    port_err_t (*read_motor_load_mv)(uint16_t *mv);
    port_err_t (*read_battery_mv)(uint16_t *mv);
} adc_port_t;

const adc_port_t *adc_port_get(void);

#endif /* ADC_PORT_H */
