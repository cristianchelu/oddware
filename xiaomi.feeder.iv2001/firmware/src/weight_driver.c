/*
 * Weight driver composition — spec/30-processes/weighing.md
 */

#include <stddef.h>

#include "weigh_cal.h"
#include "weigh_product.h"
#include "weight_driver.h"

void weight_driver_init(weight_driver_state_t *state, const weight_hw_t *hw)
{
    state->hw = *hw;
    weight_rail_ctx_init(&state->rail, hw->expander, hw->delay_ms);
    state->powered = false;
    state->boot_done = false;
    state->scale_off = false;
    (void)weigh_cal_load(hw->config, &state->cal);
}

bool weight_scale_off(const weight_driver_state_t *state)
{
    if (state == NULL) {
        return false;
    }

    return state->scale_off;
}

static port_err_t weight_require_rail_on(weight_driver_state_t *state)
{
    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (state->scale_off) {
        return PORT_ERR_NOT_FOUND;
    }

    return PORT_OK;
}

static port_err_t weight_require_booted(weight_driver_state_t *state)
{
    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!state->powered || !state->boot_done) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

port_err_t weight_rail_enable(weight_driver_state_t *state)
{
    port_err_t err;

    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (state->powered && !state->scale_off) {
        return PORT_OK;
    }

    err = weight_rail_on(&state->rail);
    if (err != PORT_OK) {
        return err;
    }

    state->powered = true;
    state->scale_off = false;
    return PORT_OK;
}

port_err_t weight_boot_settle(weight_driver_state_t *state)
{
    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (state->boot_done) {
        return PORT_OK;
    }

    if (!state->powered || state->scale_off) {
        return PORT_ERR_IO;
    }

    if (state->hw.delay_ms != NULL) {
        state->hw.delay_ms(CS1270_BOOT_MS);
    }

    state->boot_done = true;
    return PORT_OK;
}

port_err_t weight_power_off(weight_driver_state_t *state)
{
    port_err_t err;

    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = weight_rail_off(&state->rail);
    if (err == PORT_OK) {
        state->powered = false;
        state->boot_done = false;
        state->scale_off = true;
    }
    return err;
}

static port_err_t weight_query_raw(weight_driver_state_t *state,
                                   int32_t *raw,
                                   cs1270_status_t *status)
{
    uint32_t attempt;

    for (attempt = 0u; attempt < CS1270_POWER_RETRIES; attempt++) {
        cs1270_status_t st;
        port_err_t err;
        uint32_t warm;

        err = cs1270_query(state->hw.uart, raw, &st);
        if (err != PORT_OK) {
            if (state->hw.delay_ms != NULL) {
                state->hw.delay_ms(CS1270_UART_RETRY_MS);
            }
            continue;
        }

        for (warm = 0u;
             st == CS1270_STATUS_BOOT_WARMING && warm < CS1270_WARM_POLL_MAX;
             warm++) {
            if (state->hw.delay_ms != NULL) {
                state->hw.delay_ms(CS1270_POLL_MS);
            }
            err = cs1270_query(state->hw.uart, raw, &st);
            if (err != PORT_OK) {
                break;
            }
        }

        if (err != PORT_OK) {
            if (state->hw.delay_ms != NULL) {
                state->hw.delay_ms(CS1270_UART_RETRY_MS);
            }
            continue;
        }

        if (st == CS1270_STATUS_BOOT_WARMING) {
            continue;
        }

        if (status != NULL) {
            *status = st;
        }
        return PORT_OK;
    }

    return PORT_ERR_IO;
}

port_err_t weight_read_grams(weight_driver_state_t *state, int32_t *grams)
{
    int32_t raw;
    cs1270_status_t st;
    port_err_t err;

    if (state == NULL || grams == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = weight_require_rail_on(state);
    if (err != PORT_OK) {
        return err;
    }

    if (!weigh_cal_is_complete(&state->cal)) {
        return PORT_ERR_NOT_SUPPORTED;
    }

    err = weight_require_booted(state);
    if (err != PORT_OK) {
        return err;
    }

    err = weight_query_raw(state, &raw, &st);
    if (err != PORT_OK) {
        return err;
    }

    if (st != CS1270_STATUS_WEIGHT) {
        return PORT_ERR_IO;
    }

    return weigh_cal_food_grams(&state->cal, raw, grams);
}

port_err_t weight_read_raw_grams(weight_driver_state_t *state, int32_t *grams)
{
    cs1270_status_t st;
    port_err_t err;

    if (state == NULL || grams == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = weight_require_rail_on(state);
    if (err != PORT_OK) {
        return err;
    }

    err = weight_require_booted(state);
    if (err != PORT_OK) {
        return err;
    }

    err = weight_query_raw(state, grams, &st);
    if (err != PORT_OK) {
        return err;
    }

    if (st != CS1270_STATUS_WEIGHT) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

port_err_t weight_calibrate_zero(weight_driver_state_t *state)
{
    int32_t raw;
    cs1270_status_t st;
    port_err_t err;

    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = weight_require_rail_on(state);
    if (err != PORT_OK) {
        return err;
    }

    err = weight_require_booted(state);
    if (err != PORT_OK) {
        return err;
    }

    err = weight_query_raw(state, &raw, &st);
    if (err != PORT_OK) {
        return err;
    }

    if (st != CS1270_STATUS_WEIGHT) {
        return PORT_ERR_NOT_SUPPORTED;
    }

    return weigh_cal_save_zero(state->hw.config, raw, &state->cal);
}

port_err_t weight_calibrate_span(weight_driver_state_t *state)
{
    int32_t raw;
    cs1270_status_t st;
    port_err_t err;

    if (state == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!state->cal.zero_set) {
        return PORT_ERR_INVALID_ARG;
    }

    err = weight_require_rail_on(state);
    if (err != PORT_OK) {
        return err;
    }

    err = weight_require_booted(state);
    if (err != PORT_OK) {
        return err;
    }

    err = weight_query_raw(state, &raw, &st);
    if (err != PORT_OK) {
        return err;
    }

    if (st != CS1270_STATUS_WEIGHT) {
        return PORT_ERR_NOT_SUPPORTED;
    }

    return weigh_cal_save_span(state->hw.config, WEIGH_BOWL_MASS_G, raw, &state->cal);
}

weight_cal_status_t weight_driver_cal_status(const weight_driver_state_t *state)
{
    if (state == NULL) {
        return WEIGHT_CAL_IDLE;
    }

    if (weigh_cal_is_complete(&state->cal)) {
        return WEIGHT_CAL_SUCCESS;
    }

    if (weigh_cal_zero_pending_span(&state->cal)) {
        return WEIGHT_CAL_CAPTURING_SPAN;
    }

    return WEIGHT_CAL_UNCALIBRATED;
}
