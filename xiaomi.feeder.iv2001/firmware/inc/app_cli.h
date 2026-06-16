#ifndef APP_CLI_H
#define APP_CLI_H

#include "cli.h"
#include "app_cli_active.h"

void app_cli_start(void);
void app_cli_session_init(cli_t *cli, int (*get)(void), int (*put)(int));

#endif /* APP_CLI_H */
