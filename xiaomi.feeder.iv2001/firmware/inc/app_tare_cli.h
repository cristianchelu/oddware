/*
 * UART CLI: tare command table — spec/30-processes/uart-console.md § tare
 */

#ifndef APP_TARE_CLI_H
#define APP_TARE_CLI_H

#include "cli.h"

extern cmd_t tare_cli_subcmds[];

uint8_t tare_cli_run_show(void);

#endif /* APP_TARE_CLI_H */
