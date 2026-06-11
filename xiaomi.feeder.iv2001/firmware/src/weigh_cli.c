/*
 * Weigh bench CLI logic — spec/30-processes/uart-console.md
 */

#include <stdbool.h>

#include "app_log.h"
#include "weigh_cli.h"
#include "weight_port.h"

void weigh_cli_print_fail(const char *what, port_err_t err)
{
    app_log_info("cli", "weigh %s failed (%s)", what, port_err_name(err));
}

bool weigh_cli_print_scale_off(const char *what, port_err_t err)
{
    if (err != PORT_ERR_NOT_FOUND) {
        return false;
    }

    app_log_info("cli", "weigh %s: scale off (weigh power on first)", what);
    return true;
}

bool weigh_cli_print_read_fail(port_err_t err)
{
    weight_cal_status_t st;

    if (weigh_cli_print_scale_off("read", err)) {
        return true;
    }

    if (err != PORT_ERR_NOT_SUPPORTED) {
        return false;
    }

    st = weigh_cli_run_cal_status();
    if (st == WEIGHT_CAL_CAPTURING_SPAN) {
        app_log_info("cli",
                     "weigh read: calibration incomplete (install bowl, weigh cal span)");
        return true;
    }

    app_log_info("cli",
                 "weigh read: no calibration (weigh cal zero, then weigh cal span)");
    return true;
}

const char *weigh_cli_cal_status_name(weight_cal_status_t st)
{
    switch (st) {
    case WEIGHT_CAL_CAPTURING_SPAN:
        return "capturing_span";
    case WEIGHT_CAL_SUCCESS:
        return "success";
    case WEIGHT_CAL_UNCALIBRATED:
        return "uncalibrated";
    case WEIGHT_CAL_IDLE:
    default:
        return "idle";
    }
}

port_err_t weigh_cli_run_power_on(void)
{
    const weight_port_t *wp = weight_port_get();

    if (wp == NULL || wp->power_on == NULL) {
        return PORT_ERR_IO;
    }

    return wp->power_on();
}

port_err_t weigh_cli_run_power_off(void)
{
    const weight_port_t *wp = weight_port_get();

    if (wp == NULL || wp->power_off == NULL) {
        return PORT_ERR_IO;
    }

    return wp->power_off();
}

port_err_t weigh_cli_run_read(int32_t *grams)
{
    const weight_port_t *wp = weight_port_get();

    if (wp == NULL || wp->read_grams == NULL || grams == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return wp->read_grams(grams);
}

port_err_t weigh_cli_run_read_raw(int32_t *grams)
{
    const weight_port_t *wp = weight_port_get();

    if (wp == NULL || wp->read_raw_grams == NULL || grams == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return wp->read_raw_grams(grams);
}

port_err_t weigh_cli_run_cal_zero(void)
{
    const weight_port_t *wp = weight_port_get();

    if (wp == NULL || wp->calibrate_zero == NULL) {
        return PORT_ERR_IO;
    }

    return wp->calibrate_zero();
}

port_err_t weigh_cli_run_cal_span(void)
{
    const weight_port_t *wp = weight_port_get();

    if (wp == NULL || wp->calibrate_span == NULL) {
        return PORT_ERR_IO;
    }

    return wp->calibrate_span();
}

weight_cal_status_t weigh_cli_run_cal_status(void)
{
    const weight_port_t *wp = weight_port_get();

    if (wp == NULL || wp->get_cal_status == NULL) {
        return WEIGHT_CAL_IDLE;
    }

    return wp->get_cal_status();
}
