/*
 * Motor bench CLI logic — spec/30-processes/uart-console.md
 */

#ifndef MOTOR_CLI_H
#define MOTOR_CLI_H

#include <stdbool.h>
#include <stdint.h>

#include "motor_limits.h"
#include "port_err.h"

#define MOTOR_CLI_MAX_MS  MOTOR_RUN_MS_MAX

void motor_cli_print_fail(const char *verb, port_err_t err);
port_err_t motor_cli_parse_duration_ms(const char *text, uint32_t *duration_ms);
uint8_t motor_cli_handle_fwd(uint8_t argc, char *argv[]);
uint8_t motor_cli_handle_rev(uint8_t argc, char *argv[]);
void motor_cli_on_timed_run_done(void);
void motor_cli_on_timed_run_fault(void);
void motor_cli_test_reset_timed(void);
bool motor_cli_test_take_timed_line(char *buf, size_t len);
uint8_t motor_cli_handle_park(void);
void motor_cli_on_park_done(void);
void motor_cli_on_park_fault(void);
void motor_cli_test_reset_park(void);
bool motor_cli_test_take_park_line(char *buf, size_t len);

#endif /* MOTOR_CLI_H */
