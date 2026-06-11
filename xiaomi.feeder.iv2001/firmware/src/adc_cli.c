/*
 * ADC bench CLI logic — spec/30-processes/uart-console.md
 */

#include <stdio.h>

#include "adc_cli.h"
#include "app_log.h"
#include "adc_port.h"

void adc_cli_print_fail(port_err_t err)
{
    app_log_info("cli", "adc read failed (%s)", port_err_name(err));
}

void adc_cli_print_cal_fail(port_err_t err)
{
    app_log_info("cli", "adc cal failed (%s)", port_err_name(err));
}

port_err_t adc_cli_run_motor_read(uint16_t *ma)
{
    const adc_port_t *port = adc_port_get();

    if (ma == NULL || port == NULL || port->read_motor_load_ma == NULL) {
        return PORT_ERR_IO;
    }

    return port->read_motor_load_ma(ma);
}

port_err_t adc_cli_run_battery_read(uint16_t *mv)
{
    const adc_port_t *port = adc_port_get();

    if (mv == NULL || port == NULL || port->read_battery_mv == NULL) {
        return PORT_ERR_IO;
    }

    return port->read_battery_mv(mv);
}

port_err_t adc_cli_run_cal_capture(uint16_t true_mv)
{
    const adc_port_t *port = adc_port_get();

    if (port == NULL || port->cal_capture == NULL) {
        return PORT_ERR_IO;
    }

    return port->cal_capture(true_mv);
}

port_err_t adc_cli_run_cal_reset(void)
{
    const adc_port_t *port = adc_port_get();

    if (port == NULL || port->cal_reset == NULL) {
        return PORT_ERR_IO;
    }

    return port->cal_reset();
}

port_err_t adc_cli_run_cal_status(adc_cal_status_t *status)
{
    const adc_port_t *port = adc_port_get();

    if (status == NULL || port == NULL || port->get_cal_status == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return port->get_cal_status(status);
}

void adc_cli_format_cal_status(const adc_cal_status_t *status, char *buf, size_t len)
{
    adc_cal_status_t active;

    if (buf == NULL || len == 0u) {
        return;
    }

    if (status == NULL) {
        if (adc_cli_run_cal_status(&active) != PORT_OK) {
            snprintf(buf, len, "11.000 (default)");
            return;
        }
        status = &active;
    }

    if (status->customized) {
        snprintf(buf, len, "%u.%03u",
                 (unsigned)(status->scale_x1000 / 1000u),
                 (unsigned)(status->scale_x1000 % 1000u));
    } else {
        snprintf(buf, len, "11.000 (default)");
    }
}
