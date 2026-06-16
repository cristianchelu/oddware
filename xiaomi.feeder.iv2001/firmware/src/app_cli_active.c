/*
 * MiniCLI active-instance switching (UART0 vs remote telnet).
 * spec/30-processes/uart-console.md § Remote telnet console
 */

#include "app_cli_active.h"

#include <stddef.h>

#include "cli.h"

static cli_t *s_uart_cli;

void app_cli_set_uart_cli(cli_t *cli)
{
    s_uart_cli = cli;
}

void app_cli_restore_local(void)
{
    if (s_uart_cli != NULL) {
        cli_init(s_uart_cli);
    }
}

#ifdef HOST_TEST
cli_t *app_cli_test_active_cli(void)
{
    extern cli_t *cli_host_active(void);

    return cli_host_active();
}
#endif
