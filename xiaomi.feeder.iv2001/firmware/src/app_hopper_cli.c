/*
 * UART CLI: hopper command table — spec/30-processes/uart-console.md
 */

#include <stdbool.h>
#include <stdio.h>

#include "app_hopper_cli.h"
#include "beam_cli.h"

static uint8_t hopper_cli_read_cmd(uint8_t argc, char *argv[])
{
    bool beam_blocked;
    port_err_t err;

    (void)argc;
    (void)argv;

    err = beam_cli_run_hopper_read(&beam_blocked);
    if (err != PORT_OK) {
        beam_cli_print_hopper_fail(err);
        return 1;
    }

    printf("hopper beam: %s\r\n", beam_blocked ? "blocked" : "clear");
    return 0;
}

cmd_t hopper_cli_subcmds[] = {
    { "read", "hopper IR beam read", hopper_cli_read_cmd, NULL },
    { NULL, NULL, NULL, NULL },
};
