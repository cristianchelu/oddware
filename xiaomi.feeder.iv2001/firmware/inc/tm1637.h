/*
 * TM1637 protocol — spec/10-hardware/components/display-tm1637.md
 */

#ifndef TM1637_H
#define TM1637_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

/* Five payload grids (0–4); tm1637_refresh clears grid 5 — display-tm1637.md */
#define TM1637_GRID_COUNT       5u
#define TM1637_BRIGHTNESS_MAX   0x8Bu
#define TM1637_CMD_FIXED_ADDR   0x44u
#define TM1637_CMD_DISPLAY_OFF  0x80u

typedef struct tm1637_gpio_ops {
    void (*set_dio)(bool high);
    void (*set_clk)(bool high);
    void (*delay_us)(uint32_t us);
    void (*on_byte)(uint8_t byte); /* optional trace hook for host tests */
} tm1637_gpio_ops_t;

port_err_t tm1637_refresh(const tm1637_gpio_ops_t *gpio,
                          const uint8_t grids[TM1637_GRID_COUNT],
                          uint8_t brightness_cmd);

#endif /* TM1637_H */
