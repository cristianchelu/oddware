/*
 * Power bench CLI logic — spec/30-processes/uart-console.md § power commands
 */

#ifndef POWER_CLI_H
#define POWER_CLI_H

#include <stddef.h>

#include "port_err.h"
#include "power_source_input.h"

void power_cli_print_fail(port_err_t err);
port_err_t power_cli_run_show(power_source_t *source);
const char *power_cli_format_source(power_source_t source);

#endif /* POWER_CLI_H */
