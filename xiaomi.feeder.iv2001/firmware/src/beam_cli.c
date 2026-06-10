/*
 * IR broken-beam bench CLI logic — spec/30-processes/uart-console.md
 */

#include <stdio.h>

#include "beam_cli.h"
#include "hopper_ir_port.h"
#include "motor_index_port.h"

void beam_cli_print_index_fail(port_err_t err)
{
    printf("index read failed (%s)\r\n", port_err_name(err));
}

void beam_cli_print_hopper_fail(port_err_t err)
{
    printf("hopper read failed (%s)\r\n", port_err_name(err));
}

port_err_t beam_cli_run_index_read(bool *beam_open)
{
    const motor_index_port_t *port = motor_index_port_get();
    port_err_t err;
    port_err_t off_err;

    if (beam_open == NULL || port == NULL || port->set_led == NULL ||
        port->read_beam_open == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = port->set_led(true);
    if (err != PORT_OK) {
        return err;
    }

    err = port->read_beam_open(beam_open);
    off_err = port->set_led(false);
    if (err != PORT_OK) {
        return err;
    }

    return off_err;
}

port_err_t beam_cli_run_hopper_read(bool *beam_blocked)
{
    const hopper_ir_port_t *port = hopper_ir_port_get();

    if (beam_blocked == NULL || port == NULL || port->sense == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return port->sense(beam_blocked);
}
