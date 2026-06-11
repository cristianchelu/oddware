/*
 * UART CLI: index command table — spec/30-processes/uart-console.md
 */

#include <stdbool.h>
#include <stdio.h>
#include "app_log.h"

#include "app_index_cli.h"
#include "beam_cli.h"

static uint8_t index_cli_read_cmd(uint8_t argc, char *argv[])
{
    bool beam_open;
    port_err_t err;

    (void)argc;
    (void)argv;

    err = beam_cli_run_index_read(&beam_open);
    if (err != PORT_OK) {
        beam_cli_print_index_fail(err);
        return 1;
    }

    app_log_info("cli", "index beam: %s", beam_open ? "open" : "blocked");
    return 0;
}

cmd_t index_cli_subcmds[] = {
    { "read", "motor-index IR beam read", index_cli_read_cmd, NULL },
    { NULL, NULL, NULL, NULL },
};
