#ifndef DISPLAY_CLI_H
#define DISPLAY_CLI_H

#include <stdint.h>

#include "display_glyph.h"
#include "display_presentation.h"
#include "port_err.h"

port_err_t display_cli_parse_hex_byte(const char *text, uint8_t *out);
port_err_t display_cli_parse_number(const char *text, uint16_t *out);
port_err_t display_cli_parse_blink_ms(const char *text, uint16_t *out);

port_err_t display_cli_run_test(void);
port_err_t display_cli_run_fill(uint8_t segment_byte);
port_err_t display_cli_run_off(void);

port_err_t display_cli_run_number(uint16_t value, display_unit_t unit);
port_err_t display_cli_run_icon_set(display_icon_t icon, bool on);
port_err_t display_cli_run_icon_blink(display_icon_t icon,
                                    uint16_t on_ms,
                                    uint16_t off_ms);
port_err_t display_cli_run_icon_steady(display_icon_t icon);
port_err_t display_cli_run_anim(display_builtin_anim_t id, bool loop);
port_err_t display_cli_run_anim_stop(void);
port_err_t display_cli_run_brightness(uint8_t level);

void display_cli_delay_ms(uint32_t ms);
void display_cli_print_fail(const char *what, port_err_t err);

#endif /* DISPLAY_CLI_H */
