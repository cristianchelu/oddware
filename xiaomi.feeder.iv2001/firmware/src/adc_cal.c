/*
 * Battery divider calibration — spec/30-processes/battery-monitoring.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adc_cal.h"
#include "adc_limits.h"
#include "config_keys.h"

static void adc_cal_set_default(adc_cal_model_t *model)
{
    model->scale_x1000 = ADC_CAL_DEFAULT_SCALE_X1000;
    model->customized = false;
}

static port_err_t adc_cal_write_scale(const config_port_t *cfg, uint32_t scale_x1000)
{
    char buf[12];

    if (cfg == NULL || cfg->write == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    snprintf(buf, sizeof(buf), "%lu", (unsigned long)scale_x1000);
    return cfg->write(CONFIG_GROUP_POWER, CONFIG_KEY_POWER_BATT_SCALE_X1000, buf);
}

static port_err_t adc_cal_read_scale(const config_port_t *cfg, uint32_t *out)
{
    char buf[12];
    char *end;
    unsigned long parsed;

    if (cfg == NULL || cfg->read == NULL || out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (cfg->read(CONFIG_GROUP_POWER, CONFIG_KEY_POWER_BATT_SCALE_X1000,
                  buf, sizeof(buf)) != PORT_OK) {
        return PORT_ERR_NOT_FOUND;
    }

    parsed = strtoul(buf, &end, 10);
    if (end == buf || parsed == 0ul || parsed > 65535ul) {
        return PORT_ERR_IO;
    }

    *out = (uint32_t)parsed;
    return PORT_OK;
}

static bool adc_cal_scale_valid(uint32_t scale_x1000)
{
    return scale_x1000 >= (uint32_t)ADC_CAL_RATIO_MIN_X1000 &&
           scale_x1000 <= (uint32_t)ADC_CAL_RATIO_MAX_X1000;
}

static uint32_t adc_cal_scale_from_capture(uint16_t true_mv, uint16_t pin_mv)
{
    return (((uint32_t)true_mv * 1000u) + ((uint32_t)pin_mv / 2u)) /
           (uint32_t)pin_mv;
}

port_err_t adc_cal_load(const config_port_t *cfg, adc_cal_model_t *out)
{
    port_err_t err;

    if (out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    adc_cal_set_default(out);

    if (cfg == NULL) {
        return PORT_OK;
    }

    err = adc_cal_read_scale(cfg, &out->scale_x1000);
    if (err == PORT_ERR_NOT_FOUND) {
        return PORT_OK;
    }

    if (err != PORT_OK || !adc_cal_scale_valid(out->scale_x1000)) {
        adc_cal_set_default(out);
        return PORT_ERR_IO;
    }

    out->customized = true;
    return PORT_OK;
}

port_err_t adc_cal_capture(const config_port_t *cfg,
                           uint16_t true_mv,
                           uint16_t pin_mv,
                           adc_cal_model_t *model)
{
    uint32_t scale_x1000;
    port_err_t err;

    if (model == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (true_mv < ADC_CAL_TRUE_MV_MIN || true_mv > ADC_CAL_TRUE_MV_MAX) {
        return PORT_ERR_INVALID_ARG;
    }

    if (pin_mv < ADC_CAL_PIN_MV_MIN) {
        return PORT_ERR_INVALID_ARG;
    }

    scale_x1000 = adc_cal_scale_from_capture(true_mv, pin_mv);
    if (!adc_cal_scale_valid(scale_x1000)) {
        return PORT_ERR_INVALID_ARG;
    }

    if (cfg != NULL) {
        err = adc_cal_write_scale(cfg, scale_x1000);
        if (err != PORT_OK) {
            return err;
        }
    }

    model->scale_x1000 = scale_x1000;
    model->customized = true;
    return PORT_OK;
}

port_err_t adc_cal_reset(const config_port_t *cfg, adc_cal_model_t *model)
{
    if (model == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (cfg != NULL && cfg->erase != NULL) {
        (void)cfg->erase(CONFIG_GROUP_POWER, CONFIG_KEY_POWER_BATT_SCALE_X1000);
    }

    adc_cal_set_default(model);
    return PORT_OK;
}

uint16_t adc_cal_apply_pin_mv(const adc_cal_model_t *model, uint16_t pin_mv)
{
    uint32_t scale_x1000 = ADC_CAL_DEFAULT_SCALE_X1000;

    if (model != NULL) {
        scale_x1000 = model->scale_x1000;
    }

    return (uint16_t)(((uint32_t)pin_mv * scale_x1000) / 1000u);
}
