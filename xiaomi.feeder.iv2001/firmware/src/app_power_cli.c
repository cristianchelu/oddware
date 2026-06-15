/*
 * UART CLI: power command table — spec/30-processes/uart-console.md
 */

#include <stdint.h>

#include "app_log.h"
#include "app_power_cli.h"
#include "power_cli.h"

static uint8_t power_cli_show_cmd(uint8_t argc, char *argv[])
{
    power_source_t source;
    port_err_t err;

    (void)argc;
    (void)argv;

    err = power_cli_run_show(&source);
    if (err != PORT_OK) {
        power_cli_print_fail(err);
        return 1;
    }

    app_log_info("cli", "power source: %s", power_cli_format_source(source));
    return 0;
}

cmd_t power_cli_subcmds[] = {
    { "show", "debounced mains/battery source", power_cli_show_cmd, NULL },
    { NULL, NULL, NULL, NULL },
};
