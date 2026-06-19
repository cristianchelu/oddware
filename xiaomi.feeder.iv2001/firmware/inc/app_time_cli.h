#ifndef APP_TIME_CLI_H
#define APP_TIME_CLI_H

#include "cli.h"

uint8_t time_cli_run_show(void);
uint8_t time_cli_run_sync(void);

extern cmd_t time_cli_subcmds[];

#endif /* APP_TIME_CLI_H */
