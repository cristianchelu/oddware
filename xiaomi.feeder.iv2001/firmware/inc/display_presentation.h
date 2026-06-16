/*
 * Display presentation — spec/30-processes/display-presentation.md
 */

#ifndef DISPLAY_PRESENTATION_H
#define DISPLAY_PRESENTATION_H

#include <stdbool.h>
#include <stdint.h>

#include "display_glyph.h"
#include "port_err.h"
#include "tm1637.h"

#define DISPLAY_PRESENTATION_TICK_IDLE  UINT32_MAX
#define DISPLAY_PRESENTATION_BLINK_MIN_MS  50u
#define DISPLAY_PRESENTATION_BLINK_MAX_MS  5000u
#define DISPLAY_PRESENTATION_MAX_BLINKS    4u
#define DISPLAY_PRESENTATION_MAX_PATTERNS  4u
#define DISPLAY_PRESENTATION_MAX_PATTERN_PHASES  8u

typedef struct {
    uint16_t duration_ms;
    bool visible;
} display_pattern_phase_t;

typedef struct display_animation {
    const uint8_t (*frames)[TM1637_GRID_COUNT];
    uint8_t frame_count;
    uint16_t frame_ms;
} display_animation_t;

typedef enum {
    DISPLAY_ANIM_OTA,
    DISPLAY_BUILTIN_ANIM_COUNT
} display_builtin_anim_t;

void display_presentation_reset(void);
void display_presentation_note_expander_reset(void);

port_err_t display_presentation_set_digits(uint16_t value);
port_err_t display_presentation_set_digits_dash(void);
port_err_t display_presentation_clear_digits(void);
port_err_t display_presentation_set_unit(display_unit_t unit);
port_err_t display_presentation_icon_set(display_icon_t icon, bool on);
port_err_t display_presentation_set_brightness(uint8_t level);

port_err_t display_presentation_icon_blink(display_icon_t icon,
                                         uint16_t on_ms,
                                         uint16_t off_ms);
port_err_t display_presentation_icon_blink_stop(display_icon_t icon);

port_err_t display_presentation_icon_pattern(display_icon_t icon,
                                             const display_pattern_phase_t *phases,
                                             uint8_t phase_count,
                                             bool loop);
port_err_t display_presentation_icon_pattern_stop(display_icon_t icon);

port_err_t display_presentation_play_animation(const display_animation_t *anim,
                                               bool loop);
port_err_t display_presentation_play_builtin(display_builtin_anim_t id, bool loop);
port_err_t display_presentation_stop_animation(void);

port_err_t display_presentation_power_on(void);
port_err_t display_presentation_power_off(void);
port_err_t display_presentation_refresh(void);
uint32_t display_presentation_tick(uint32_t now_ms);

bool display_presentation_parse_icon(const char *name, display_icon_t *out);
bool display_presentation_parse_builtin_anim(const char *name,
                                             display_builtin_anim_t *out);

#endif /* DISPLAY_PRESENTATION_H */
