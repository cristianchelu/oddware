/*
 * IR broken-beam bench CLI logic — spec/30-processes/uart-console.md
 */

#ifndef BEAM_CLI_H
#define BEAM_CLI_H

#include <stdbool.h>

#include "port_err.h"

void beam_cli_print_index_fail(port_err_t err);
void beam_cli_print_hopper_fail(port_err_t err);

port_err_t beam_cli_run_index_read(bool *beam_open);
port_err_t beam_cli_run_hopper_read(bool *beam_blocked);

#endif /* BEAM_CLI_H */
