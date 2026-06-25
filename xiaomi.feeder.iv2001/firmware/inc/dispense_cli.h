/*
 * Dispense UART CLI — spec/30-processes/uart-console.md § dispense
 */

#ifndef DISPENSE_CLI_H
#define DISPENSE_CLI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "port_err.h"
#include "dispense.h"

uint8_t dispense_cli_handle_default(uint8_t argc, char *argv[]);
uint8_t dispense_cli_handle_portions(uint8_t argc, char *argv[]);
uint8_t dispense_cli_handle_grams(uint8_t argc, char *argv[]);
port_err_t dispense_cli_parse_portions(const char *text, uint8_t *out);
port_err_t dispense_cli_parse_grams(const char *text, uint8_t *out);
bool dispense_cli_on_job_done(void);
bool dispense_cli_on_job_fault(dispense_outcome_t outcome);
void dispense_cli_cancel_wait(void);
void dispense_cli_test_reset(void);
bool dispense_cli_test_take_line(char *buf, size_t len);

#endif /* DISPENSE_CLI_H */
