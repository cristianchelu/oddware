#ifndef FAKE_DISPLAY_PORT_H
#define FAKE_DISPLAY_PORT_H

#include <stddef.h>
#include <stdint.h>

#include "display_port.h"
#include "tm1637.h"

typedef enum {
    FAKE_DISPLAY_OP_POWER_ON = 0,
    FAKE_DISPLAY_OP_POWER_OFF,
    FAKE_DISPLAY_OP_SHOW_FILL,
    FAKE_DISPLAY_OP_SHOW_GRIDS,
    FAKE_DISPLAY_OP_BLANK,
    FAKE_DISPLAY_OP_SET_BRIGHTNESS,
} fake_display_op_kind_t;

typedef struct fake_display_op {
    fake_display_op_kind_t kind;
    uint8_t segment_byte;
    uint8_t grids[TM1637_GRID_COUNT];
    uint8_t brightness_level;
} fake_display_op_t;

#include "port_err.h"

void fake_display_port_reset(void);
void fake_display_port_set_power_on_err(port_err_t err);
void fake_display_port_set_show_fill_err(port_err_t err);
void fake_display_port_set_show_grids_err(port_err_t err);
const fake_display_op_t *fake_display_port_ops(size_t *count);
const fake_display_op_t *fake_display_port_last_grids(uint8_t grids[TM1637_GRID_COUNT]);
uint8_t fake_display_port_brightness(void);
const display_port_t *fake_display_port_get(void);

#endif /* FAKE_DISPLAY_PORT_H */
