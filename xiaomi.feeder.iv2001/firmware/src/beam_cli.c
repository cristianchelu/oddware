/*
 * IR broken-beam bench CLI logic — spec/30-processes/uart-console.md
 */

#include "app_log.h"
#include "beam_cli.h"
#include "hopper_ir_port.h"
#include "motor_index_port.h"

void beam_cli_print_index_fail(port_err_t err)
{
    app_log_info("cli", "index read failed (%s)", port_err_name(err));
}

void beam_cli_print_hopper_fail(port_err_t err)
{
    app_log_info("cli", "hopper read failed (%s)", port_err_name(err));
}

port_err_t beam_cli_run_index_read(bool *beam_open)
{
    const motor_index_port_t *port = motor_index_port_get();

    if (beam_open == NULL || port == NULL || port->sense == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return port->sense(beam_open);
}

port_err_t beam_cli_run_hopper_read(bool *beam_blocked)
{
    const hopper_ir_port_t *port = hopper_ir_port_get();

    if (beam_blocked == NULL || port == NULL || port->sense == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return port->sense(beam_blocked);
}
