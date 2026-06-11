/*
 * ADC mux + sample driver — spec/30-processes/battery-monitoring.md
 */

#include <stddef.h>

#include "adc_cal.h"
#include "adc_driver.h"
#include "adc_limits.h"
#include "board_gpio_iv2001.h"

void adc_driver_init(adc_driver_state_t *state, const adc_hw_t *hw)
{
    if (state == NULL || hw == NULL) {
        return;
    }

    state->hw = *hw;
    if (adc_cal_load(hw->config, &state->cal) != PORT_OK) {
        (void)adc_cal_reset(NULL, &state->cal);
    }
}

uint16_t adc_driver_raw_to_mv(uint16_t raw)
{
    uint32_t mv = (uint32_t)raw * (uint32_t)ADC_REF_MV;

    return (uint16_t)(mv / (uint32_t)ADC_MAX_RAW);
}

static port_err_t adc_mux_select_motor(const adc_driver_state_t *state)
{
    if (state == NULL || state->hw.expander == NULL ||
        state->hw.expander->set_pin == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return state->hw.expander->set_pin(BOARD_GPIO_ADC_MUX_PORT,
                                       BOARD_GPIO_ADC_MUX_PIN,
                                       false);
}

static port_err_t adc_mux_select_battery(const adc_driver_state_t *state)
{
    if (state == NULL || state->hw.expander == NULL ||
        state->hw.expander->set_pin == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return state->hw.expander->set_pin(BOARD_GPIO_ADC_MUX_PORT,
                                       BOARD_GPIO_ADC_MUX_PIN,
                                       true);
}

static port_err_t adc_motor_en_is_high(const adc_driver_state_t *state,
                                       bool *en_high)
{
    bool level;

    if (en_high == NULL || state == NULL || state->hw.expander == NULL ||
        state->hw.expander->get_pin == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (state->hw.expander->get_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                    BOARD_GPIO_MOTOR_EN_PIN,
                                    &level) != PORT_OK) {
        return PORT_ERR_IO;
    }

    *en_high = level;
    return PORT_OK;
}

static void adc_delay_mux_settle(const adc_driver_state_t *state)
{
    if (state != NULL && state->hw.delay_ms != NULL) {
        state->hw.delay_ms(ADC_MUX_SETTLE_MS);
    }
}

static port_err_t adc_read_one_mv(const adc_driver_state_t *state, uint16_t *mv)
{
    uint16_t raw;
    port_err_t err;

    if (mv == NULL || state == NULL || state->hw.read_raw == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = state->hw.read_raw(&raw);
    if (err != PORT_OK) {
        return err;
    }

    *mv = adc_driver_raw_to_mv(raw);
    return PORT_OK;
}

static uint16_t adc_average_trimmed(const uint16_t *samples, uint8_t count)
{
    uint32_t sum = 0u;
    uint32_t mean;
    uint32_t var_sum = 0u;
    uint32_t std_sq;
    uint8_t kept = 0u;
    uint8_t i;

    if (samples == NULL || count == 0u) {
        return 0u;
    }

    for (i = 0u; i < count; i++) {
        sum += (uint32_t)samples[i];
    }
    mean = sum / (uint32_t)count;

    if (count < 4u) {
        return (uint16_t)mean;
    }

    for (i = 0u; i < count; i++) {
        int32_t delta = (int32_t)samples[i] - (int32_t)mean;
        uint32_t sq = (uint32_t)(delta * delta);

        var_sum += sq;
    }

    std_sq = var_sum / (uint32_t)count;
    if (std_sq == 0u) {
        return (uint16_t)mean;
    }

    sum = 0u;
    for (i = 0u; i < count; i++) {
        int32_t delta = (int32_t)samples[i] - (int32_t)mean;
        uint32_t sq = (uint32_t)(delta * delta);

        if (sq <= (4u * std_sq)) {
            sum += (uint32_t)samples[i];
            kept++;
        }
    }

    if (kept == 0u) {
        return (uint16_t)mean;
    }

    return (uint16_t)(sum / (uint32_t)kept);
}

port_err_t adc_driver_read_motor_load_ma(adc_driver_state_t *state,
                                         uint16_t *ma)
{
    port_err_t err;

    if (ma == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = adc_mux_select_motor(state);
    if (err != PORT_OK) {
        return err;
    }

    adc_delay_mux_settle(state);
    return adc_read_one_mv(state, ma);
}

static port_err_t adc_read_battery_pin_mv(adc_driver_state_t *state, uint16_t *mv)
{
    bool en_high = false;
    uint16_t samples[ADC_BATTERY_SAMPLE_CNT];
    uint8_t i;
    port_err_t err;

    if (mv == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = adc_motor_en_is_high(state, &en_high);
    if (err != PORT_OK) {
        return err;
    }

    if (en_high) {
        return PORT_ERR_BUSY;
    }

    err = adc_mux_select_battery(state);
    if (err != PORT_OK) {
        return err;
    }

    adc_delay_mux_settle(state);

    for (i = 0u; i < ADC_BATTERY_SAMPLE_CNT; i++) {
        err = adc_read_one_mv(state, &samples[i]);
        if (err != PORT_OK) {
            goto restore_mux;
        }
    }

    *mv = adc_average_trimmed(samples, ADC_BATTERY_SAMPLE_CNT);
    err = PORT_OK;

restore_mux:
    if (adc_mux_select_motor(state) != PORT_OK && err == PORT_OK) {
        return PORT_ERR_IO;
    }

    return err;
}

port_err_t adc_driver_read_battery_mv(adc_driver_state_t *state, uint16_t *mv)
{
    uint16_t pin_mv;
    port_err_t err;

    if (mv == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = adc_read_battery_pin_mv(state, &pin_mv);
    if (err != PORT_OK) {
        return err;
    }

    *mv = adc_cal_apply_pin_mv(&state->cal, pin_mv);
    return PORT_OK;
}

port_err_t adc_driver_cal_capture(adc_driver_state_t *state, uint16_t true_mv)
{
    uint16_t pin_mv;
    port_err_t err;

    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = adc_read_battery_pin_mv(state, &pin_mv);
    if (err != PORT_OK) {
        return err;
    }

    return adc_cal_capture(state->hw.config, true_mv, pin_mv, &state->cal);
}

port_err_t adc_driver_cal_reset(adc_driver_state_t *state)
{
    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return adc_cal_reset(state->hw.config, &state->cal);
}

void adc_driver_cal_status(const adc_driver_state_t *state,
                           adc_cal_status_t *status)
{
    if (status == NULL) {
        return;
    }

    if (state == NULL) {
        status->scale_x1000 = ADC_CAL_DEFAULT_SCALE_X1000;
        status->customized = false;
        return;
    }

    status->scale_x1000 = state->cal.scale_x1000;
    status->customized = state->cal.customized;
}
