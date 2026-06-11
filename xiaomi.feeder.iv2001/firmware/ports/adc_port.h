/*
 * ADC sense port — spec/40-architecture/ports.md
 */

#ifndef ADC_PORT_H
#define ADC_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

typedef struct adc_cal_status {
    uint32_t scale_x1000;
    bool customized;
} adc_cal_status_t;

typedef struct adc_port {
    port_err_t (*read_motor_load_ma)(uint16_t *ma);
    port_err_t (*try_read_motor_load_ma)(uint16_t *ma);
    port_err_t (*read_battery_mv)(uint16_t *mv);
    port_err_t (*cal_capture)(uint16_t true_mv);
    port_err_t (*cal_reset)(void);
    port_err_t (*get_cal_status)(adc_cal_status_t *status);
} adc_port_t;

const adc_port_t *adc_port_get(void);

#endif /* ADC_PORT_H */
