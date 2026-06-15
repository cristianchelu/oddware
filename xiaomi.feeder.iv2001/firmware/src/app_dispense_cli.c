/*
 * UART CLI: dispense command table — spec/30-processes/uart-console.md
 */

#include <stddef.h>

#include "app_dispense_cli.h"
#include "cli.h"
#include "dispense_cli.h"

static uint8_t dispense_cli_portions_cmd(uint8_t argc, char *argv[])
{
    return dispense_cli_handle_portions(argc, argv);
}

cmd_t dispense_cli_subcmds[] = {
    { "portions", "dispense portions <N>", dispense_cli_portions_cmd, NULL },
    { NULL, NULL, NULL, NULL },
};
