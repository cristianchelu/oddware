/*
 * Host stub for app_cli_session_init — app_cli.c is ARM-only (UART HAL).
 * cli_init() activation matches the device path after history bind.
 */

#include "app_cli.h"

#include "cli.h"

void app_cli_session_init(cli_t *cli, int (*get)(void), int (*put)(int))
{
    cli->state = 1;
    cli->echo  = 0;
    cli->get   = get;
    cli->put   = put;
    cli_init(cli);
}
