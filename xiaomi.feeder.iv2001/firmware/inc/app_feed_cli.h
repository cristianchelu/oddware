/*
 * UART CLI: feed commands — spec/30-processes/uart-console.md § feed
 */

#ifndef APP_FEED_CLI_H
#define APP_FEED_CLI_H

#include <stdbool.h>
#include <stdint.h>

#include "cli.h"
#include "dispense.h"

uint8_t feed_cli_run_mode_show(void);
uint8_t feed_cli_run_mode_set(dispense_mode_t mode);
uint8_t feed_cli_run_overfill_show(void);
uint8_t feed_cli_run_overfill_set(bool enabled);
uint8_t feed_cli_run_overfill_g_show(void);
uint8_t feed_cli_run_overfill_g_set(uint8_t threshold_g);

extern cmd_t feed_cli_subcmds[];

#endif /* APP_FEED_CLI_H */
