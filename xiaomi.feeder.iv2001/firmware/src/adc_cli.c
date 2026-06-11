/*
 * ADC bench CLI logic — spec/30-processes/uart-console.md
 */

#include <stdio.h>

#include "adc_cli.h"
#include "adc_port.h"

void adc_cli_print_fail(port_err_t err)
{
    printf("adc read failed (%s)\r\n", port_err_name(err));
}

port_err_t adc_cli_run_motor_read(uint16_t *mv)
{
    const adc_port_t *port = adc_port_get();

    if (mv == NULL || port == NULL || port->read_motor_load_mv == NULL) {
        return PORT_ERR_IO;
    }

    return port->read_motor_load_mv(mv);
}

port_err_t adc_cli_run_battery_read(uint16_t *mv)
{
    const adc_port_t *port = adc_port_get();

    if (mv == NULL || port == NULL || port->read_battery_mv == NULL) {
        return PORT_ERR_IO;
    }

    return port->read_battery_mv(mv);
}
