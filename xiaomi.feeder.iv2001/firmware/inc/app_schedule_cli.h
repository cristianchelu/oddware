#ifndef APP_SCHEDULE_CLI_H
#define APP_SCHEDULE_CLI_H

#include <stdbool.h>
#include <stdint.h>

#include "cli.h"

uint8_t schedule_cli_run_show(void);
uint8_t schedule_cli_run_next(void);
uint8_t schedule_cli_run_set(unsigned hour,
                             unsigned min,
                             unsigned days,
                             unsigned g,
                             bool enabled);
uint8_t schedule_cli_run_delete(unsigned hour, unsigned min);
uint8_t schedule_cli_run_toggle(unsigned hour, unsigned min);
uint8_t schedule_cli_run_skip(unsigned hour, unsigned min, bool skip);
uint8_t schedule_cli_run_enable(bool enabled);
uint8_t schedule_cli_run_today(bool enabled);

extern cmd_t schedule_cli_subcmds[];

#endif /* APP_SCHEDULE_CLI_H */
