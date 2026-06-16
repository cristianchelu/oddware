#ifndef APP_CLI_ACTIVE_H
#define APP_CLI_ACTIVE_H

#include "cli.h"

void app_cli_set_uart_cli(cli_t *cli);
void app_cli_restore_local(void);

#ifdef HOST_TEST
cli_t *app_cli_test_active_cli(void);
#endif

#endif /* APP_CLI_ACTIVE_H */
