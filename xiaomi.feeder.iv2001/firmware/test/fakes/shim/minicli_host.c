/*
 * Host MiniCLI shim — tracks which cli_t cli_init() last activated.
 */

#include "cli.h"

static cli_t *s_active_cli;

void cli_init(cli_t *cb)
{
    s_active_cli = cb;
}

cli_t *cli_host_active(void)
{
    return s_active_cli;
}
