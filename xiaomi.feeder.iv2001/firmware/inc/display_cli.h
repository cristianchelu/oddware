#ifndef DISPLAY_CLI_H
#define DISPLAY_CLI_H

#include <stdint.h>

#include "port_err.h"

port_err_t display_cli_parse_hex_byte(const char *text, uint8_t *out);
port_err_t display_cli_run_test(void);
port_err_t display_cli_run_fill(uint8_t segment_byte);
port_err_t display_cli_run_off(void);

void display_cli_delay_ms(uint32_t ms);

#endif /* DISPLAY_CLI_H */
