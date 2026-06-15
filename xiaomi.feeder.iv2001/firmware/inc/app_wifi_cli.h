#ifndef APP_WIFI_CLI_H
#define APP_WIFI_CLI_H

#include <stdint.h>

#include "cli.h"

uint8_t wifi_cli_run_disconnect(void);

extern cmd_t wifi_cli_subcmds[];

#endif /* APP_WIFI_CLI_H */
