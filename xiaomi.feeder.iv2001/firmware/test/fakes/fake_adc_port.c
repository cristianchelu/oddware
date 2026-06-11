#include <stddef.h>

#include "adc_cal.h"
#include "fake_adc_port.h"
#include "fake_config_port.h"

static uint16_t s_motor_ma;
static uint16_t s_battery_pin_mv;
static uint16_t s_battery_mv;
static port_err_t s_motor_err;
static port_err_t s_battery_err;
static adc_cal_model_t s_cal_model;

static port_err_t fake_read_motor_load_ma(uint16_t *ma)
{
    if (ma == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_motor_err != PORT_OK) {
        return s_motor_err;
    }

    *ma = s_motor_ma;
    return PORT_OK;
}

static port_err_t fake_read_battery_mv(uint16_t *mv)
{
    if (mv == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_battery_err != PORT_OK) {
        return s_battery_err;
    }

    *mv = s_battery_mv;
    return PORT_OK;
}

static port_err_t fake_cal_capture(uint16_t true_mv)
{
    if (s_battery_err != PORT_OK) {
        return s_battery_err;
    }

    return adc_cal_capture(fake_config_port_get(), true_mv, s_battery_pin_mv,
                           &s_cal_model);
}

static port_err_t fake_cal_reset(void)
{
    return adc_cal_reset(fake_config_port_get(), &s_cal_model);
}

static port_err_t fake_get_cal_status(adc_cal_status_t *status)
{
    if (status == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    status->scale_x1000 = s_cal_model.scale_x1000;
    status->customized = s_cal_model.customized;
    return PORT_OK;
}

static const adc_port_t s_fake_adc = {
    .read_motor_load_ma = fake_read_motor_load_ma,
    .read_battery_mv = fake_read_battery_mv,
    .cal_capture = fake_cal_capture,
    .cal_reset = fake_cal_reset,
    .get_cal_status = fake_get_cal_status,
};

void fake_adc_port_reset(void)
{
    s_motor_ma = 0u;
    s_battery_pin_mv = 0u;
    s_battery_mv = 0u;
    s_motor_err = PORT_OK;
    s_battery_err = PORT_OK;
    (void)adc_cal_reset(NULL, &s_cal_model);
}

void fake_adc_port_set_motor_ma(uint16_t ma)
{
    s_motor_ma = ma;
}

void fake_adc_port_set_battery_pin_mv(uint16_t mv)
{
    s_battery_pin_mv = mv;
}

void fake_adc_port_set_battery_mv(uint16_t mv)
{
    s_battery_mv = mv;
}

void fake_adc_port_set_motor_err(port_err_t err)
{
    s_motor_err = err;
}

void fake_adc_port_set_battery_err(port_err_t err)
{
    s_battery_err = err;
}

const adc_port_t *fake_adc_port_get(void)
{
    return &s_fake_adc;
}

const adc_port_t *adc_port_get(void)
{
    return fake_adc_port_get();
}
