/*
 * UART CLI: feed commands — spec/30-processes/uart-console.md § feed
 */

#ifndef APP_FEED_CLI_H
#define APP_FEED_CLI_H

#include <stdint.h>

#include "cli.h"
#include "dispense.h"

uint8_t feed_cli_run_mode_show(void);
uint8_t feed_cli_run_mode_set(dispense_mode_t mode);

extern cmd_t feed_cli_subcmds[];

#endif /* APP_FEED_CLI_H */
