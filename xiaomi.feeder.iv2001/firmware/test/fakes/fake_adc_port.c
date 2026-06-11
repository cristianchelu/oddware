#include <stddef.h>

#include "fake_adc_port.h"

static uint16_t s_motor_mv;
static uint16_t s_battery_mv;
static port_err_t s_motor_err;
static port_err_t s_battery_err;

static port_err_t fake_read_motor_load_mv(uint16_t *mv)
{
    if (mv == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_motor_err != PORT_OK) {
        return s_motor_err;
    }

    *mv = s_motor_mv;
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

static const adc_port_t s_fake_adc = {
    .read_motor_load_mv = fake_read_motor_load_mv,
    .read_battery_mv = fake_read_battery_mv,
};

void fake_adc_port_reset(void)
{
    s_motor_mv = 0u;
    s_battery_mv = 0u;
    s_motor_err = PORT_OK;
    s_battery_err = PORT_OK;
}

void fake_adc_port_set_motor_mv(uint16_t mv)
{
    s_motor_mv = mv;
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
