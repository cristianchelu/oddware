/*
 * Dispense UART CLI — spec/30-processes/uart-console.md § dispense
 */

#ifndef DISPENSE_CLI_H
#define DISPENSE_CLI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint8_t dispense_cli_handle(uint8_t argc, char *argv[]);
bool dispense_cli_on_burst_done(void);
bool dispense_cli_on_motor_fault(void);
void dispense_cli_cancel_wait(void);
void dispense_cli_test_reset(void);
bool dispense_cli_test_take_line(char *buf, size_t len);

#endif /* DISPENSE_CLI_H */
