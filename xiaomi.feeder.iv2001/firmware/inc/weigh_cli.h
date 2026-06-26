/*
 * Weigh bench CLI logic — spec/30-processes/uart-console.md
 */

#ifndef WEIGH_CLI_H
#define WEIGH_CLI_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"
#include "weight_port.h"

void weigh_cli_print_fail(const char *what, port_err_t err);
bool weigh_cli_print_scale_off(const char *what, port_err_t err);
bool weigh_cli_print_read_fail(port_err_t err);
const char *weigh_cli_cal_status_name(weight_cal_status_t st);

port_err_t weigh_cli_run_power_on(void);
port_err_t weigh_cli_run_power_off(void);
port_err_t weigh_cli_run_read(weight_dg_t *dg);
port_err_t weigh_cli_run_read_raw(int32_t *grams);
port_err_t weigh_cli_run_cal_zero(void);
port_err_t weigh_cli_run_cal_span(void);
weight_cal_status_t weigh_cli_run_cal_status(void);

#endif /* WEIGH_CLI_H */
