/*
 * Display glyph composition — spec/30-processes/display-presentation.md
 */

#ifndef DISPLAY_GLYPH_H
#define DISPLAY_GLYPH_H

#include <stdint.h>

#include "tm1637.h"

typedef enum {
    DISPLAY_ICON_CHILD_LOCK,
    DISPLAY_ICON_WIFI,
    DISPLAY_ICON_DISPENSING,
    DISPLAY_ICON_PERCENT,
    DISPLAY_ICON_GRAM,
    DISPLAY_ICON_BLOCKAGE,
    DISPLAY_ICON_INSUFFICIENT_FOOD,
    DISPLAY_ICON_BOWL_ERROR,
    DISPLAY_ICON_BAR_ORANGE,
    DISPLAY_ICON_BAR_GREEN,
    DISPLAY_ICON_COUNT
} display_icon_t;

typedef enum {
    DISPLAY_UNIT_NONE,
    DISPLAY_UNIT_PERCENT,
    DISPLAY_UNIT_GRAM,
} display_unit_t;

#define DISPLAY_GLYPH_ICON_MASK(icon) (1u << (unsigned)(icon))

void display_compose_grids(bool show_digits,
                           uint16_t value,
                           display_unit_t unit,
                           uint32_t icon_mask,
                           uint8_t out[TM1637_GRID_COUNT]);

uint8_t display_glyph_digit_segment(uint8_t digit);

#endif /* DISPLAY_GLYPH_H */
