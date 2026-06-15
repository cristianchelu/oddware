/*
 * Power bench CLI logic — spec/30-processes/uart-console.md § power commands
 */

#include <stddef.h>

#include "app_log.h"
#include "power_cli.h"
#include "power_source_input.h"

void power_cli_print_fail(port_err_t err)
{
    app_log_info("cli", "power show failed (%s)", port_err_name(err));
}

const char *power_cli_format_source(power_source_t source)
{
    if (source == POWER_SOURCE_MAINS) {
        return "mains";
    }

    return "battery";
}

port_err_t power_cli_run_show(power_source_t *source)
{
    if (source == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!power_source_input_is_valid()) {
        return PORT_ERR_IO;
    }

    *source = power_source_input_get();
    return PORT_OK;
}
