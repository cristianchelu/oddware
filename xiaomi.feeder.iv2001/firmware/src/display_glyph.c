/*
 * Display glyph composition — spec/30-processes/display-presentation.md
 */

#include <stddef.h>

#include "display_glyph.h"

static const uint8_t s_digit_lut[] = {
    0x3Fu, 0x06u, 0x5Bu, 0x4Fu, 0x66u, 0x6Du, 0x7Du, 0x07u, 0x7Fu, 0x6Fu,
};

static const uint8_t s_icon_grid3_mask[] = {
    0x01u, /* CHILD_LOCK */
    0x02u, /* WIFI */
    0x04u, /* DISPENSING */
    0x08u, /* PERCENT */
    0x10u, /* GRAM */
    0x20u, /* BLOCKAGE */
    0x40u, /* INSUFFICIENT_FOOD */
    0x00u, /* BOWL_ERROR — grid 4 */
    0x00u, /* BAR_ORANGE — grid 4 */
    0x00u, /* BAR_GREEN — grid 4 */
};

static const uint8_t s_icon_grid4_mask[] = {
    0x00u,
    0x00u,
    0x00u,
    0x00u,
    0x00u,
    0x00u,
    0x00u,
    0x04u, /* BOWL_ERROR */
    0x01u, /* BAR_ORANGE */
    0x02u, /* BAR_GREEN */
};

uint8_t display_glyph_digit_segment(uint8_t digit)
{
    if (digit > 9u) {
        return 0x00u;
    }
    return s_digit_lut[digit];
}

void display_compose_grids(bool show_digits,
                           uint16_t value,
                           display_unit_t unit,
                           uint32_t icon_mask,
                           uint8_t out[TM1637_GRID_COUNT])
{
    uint8_t hundreds;
    uint8_t tens;
    uint8_t ones;
    uint8_t grid3 = 0u;
    uint8_t grid4 = 0u;

    if (out == NULL) {
        return;
    }

    if (show_digits) {
        if (value > 999u) {
            value = 999u;
        }

        hundreds = (uint8_t)((value / 100u) % 10u);
        tens = (uint8_t)((value / 10u) % 10u);
        ones = (uint8_t)(value % 10u);

        out[0] = (value >= 100u) ? display_glyph_digit_segment(hundreds) : 0x00u;
        out[1] = (value >= 10u) ? display_glyph_digit_segment(tens) : 0x00u;
        out[2] = display_glyph_digit_segment(ones);
    } else {
        out[0] = 0x00u;
        out[1] = 0x00u;
        out[2] = 0x00u;
    }

    if (unit == DISPLAY_UNIT_PERCENT) {
        grid3 |= 0x08u;
    } else if (unit == DISPLAY_UNIT_GRAM) {
        grid3 |= 0x10u;
    }

    for (display_icon_t icon = 0; icon < DISPLAY_ICON_COUNT; icon++) {
        if ((icon_mask & DISPLAY_GLYPH_ICON_MASK(icon)) == 0u) {
            continue;
        }
        grid3 |= s_icon_grid3_mask[icon];
        grid4 |= s_icon_grid4_mask[icon];
    }

    out[3] = grid3;
    out[4] = grid4;
}
