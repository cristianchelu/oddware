/*
 * Battery divider calibration — spec/30-processes/battery-monitoring.md
 */

#ifndef ADC_CAL_H
#define ADC_CAL_H

#include <stdbool.h>
#include <stdint.h>

#include "config_port.h"
#include "port_err.h"

#define ADC_CAL_DEFAULT_SCALE_X1000  11000u

typedef struct adc_cal_model {
    uint32_t scale_x1000;
    bool customized;
} adc_cal_model_t;

port_err_t adc_cal_load(const config_port_t *cfg, adc_cal_model_t *out);
port_err_t adc_cal_capture(const config_port_t *cfg,
                           uint16_t true_mv,
                           uint16_t pin_mv,
                           adc_cal_model_t *model);
port_err_t adc_cal_reset(const config_port_t *cfg, adc_cal_model_t *model);
uint16_t adc_cal_apply_pin_mv(const adc_cal_model_t *model, uint16_t pin_mv);

#endif /* ADC_CAL_H */
