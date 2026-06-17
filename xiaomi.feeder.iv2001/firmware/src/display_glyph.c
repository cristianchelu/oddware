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

typedef struct {
    uint8_t grid;
    uint8_t mask;
} display_glyph_ota_path_step_t;

static const display_glyph_ota_path_step_t s_ota_path[] = {
    { 0u, 0x01u }, /* Hundreds A */
    { 1u, 0x01u }, /* Tens A */
    { 2u, 0x01u }, /* Singles A */
    { 2u, 0x02u }, /* Singles B */
    { 2u, 0x04u }, /* Singles C */
    { 2u, 0x08u }, /* Singles D */
    { 1u, 0x08u }, /* Tens D */
    { 0u, 0x08u }, /* Hundreds D */
    { 0u, 0x10u }, /* Hundreds E */
    { 0u, 0x20u }, /* Hundreds F */
};

uint8_t display_glyph_ota_filled_from_pct(uint8_t pct)
{
    uint16_t filled;

    if (pct == 0u) {
        return 0u;
    }

    filled = (uint16_t)(pct + 9u) / 10u;
    if (filled > DISPLAY_GLYPH_OTA_PATH_LEN) {
        filled = DISPLAY_GLYPH_OTA_PATH_LEN;
    }
    return (uint8_t)filled;
}

void display_glyph_ota_bar(uint8_t filled_segments, bool g_on,
                           uint8_t out[TM1637_GRID_COUNT])
{
    uint8_t i;

    if (out == NULL) {
        return;
    }

    out[0] = 0x00u;
    out[1] = 0x00u;
    out[2] = 0x00u;
    out[3] = 0x00u;
    out[4] = 0x00u;

    if (filled_segments > DISPLAY_GLYPH_OTA_PATH_LEN) {
        filled_segments = DISPLAY_GLYPH_OTA_PATH_LEN;
    }

    for (i = 0u; i < filled_segments; i++) {
        out[s_ota_path[i].grid] |= s_ota_path[i].mask;
    }

    if (g_on) {
        out[0] |= 0x40u;
        out[1] |= 0x40u;
        out[2] |= 0x40u;
    }
}
