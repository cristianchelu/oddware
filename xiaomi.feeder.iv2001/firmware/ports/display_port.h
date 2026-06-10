/*
 * Display port — spec/40-architecture/ports.md
 */

#ifndef DISPLAY_PORT_H
#define DISPLAY_PORT_H

#include <stdint.h>

#include "port_err.h"
#include "tm1637.h"

typedef struct display_port {
    port_err_t (*power_on)(void);
    port_err_t (*power_off)(void);
    port_err_t (*show_fill)(uint8_t segment_byte);
    port_err_t (*show_grids)(const uint8_t grids[TM1637_GRID_COUNT]);
    port_err_t (*blank)(void);
    port_err_t (*set_brightness)(uint8_t level);
} display_port_t;

const display_port_t *display_port_get(void);

#endif /* DISPLAY_PORT_H */
