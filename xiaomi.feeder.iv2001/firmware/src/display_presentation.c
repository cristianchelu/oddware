/*
 * Display presentation — spec/30-processes/display-presentation.md
 */

#include <stddef.h>
#include <string.h>

#include "display_anim_builtin.h"
#include "display_glyph.h"
#include "display_presentation.h"
#include "display_port.h"

#define DISPLAY_OTA_CONNECTING_BLINK_MS  300u
#define DISPLAY_OTA_VERIFYING_BLINK_MS   200u

typedef struct {
    bool active;
    display_icon_t icon;
    uint16_t on_ms;
    uint16_t off_ms;
    bool phase_on;
    uint32_t phase_until_ms;
} display_blink_slot_t;

typedef struct {
    bool active;
    display_icon_t icon;
    bool loop;
    uint8_t phase_count;
    display_pattern_phase_t phases[DISPLAY_PRESENTATION_MAX_PATTERN_PHASES];
    uint8_t phase_index;
    bool phase_visible;
    uint32_t phase_until_ms;
} display_pattern_slot_t;

static struct {
    uint16_t digits;
    bool digits_valid;
    bool digits_dash;
    bool digits_underflow;
    display_unit_t unit;
    uint32_t steady_icons;
    uint8_t brightness;
    bool powered;

    display_blink_slot_t blinks[DISPLAY_PRESENTATION_MAX_BLINKS];
    display_pattern_slot_t patterns[DISPLAY_PRESENTATION_MAX_PATTERNS];

    const display_animation_t *anim;
    bool anim_loop;
    uint8_t anim_frame;
    uint32_t anim_next_ms;
    bool anim_running;

    bool ota_active;
    display_ota_phase_t ota_phase;
    uint8_t ota_pct;
    bool ota_blink_on;
    uint32_t ota_blink_until_ms;

    uint32_t last_now_ms;
    bool scene_dirty;
} s_pres;

static void presentation_mark_scene_dirty(void)
{
    s_pres.scene_dirty = true;
}

static const char *const s_icon_names[DISPLAY_ICON_COUNT] = {
    "child_lock",
    "wifi",
    "dispensing",
    "percent",
    "gram",
    "blockage",
    "insufficient_food",
    "bowl_error",
    "bar_orange",
    "bar_green",
};

static port_err_t presentation_ensure_power(void)
{
    const display_port_t *dp = display_port_get();
    port_err_t err;

    if (dp == NULL) {
        return PORT_ERR_IO;
    }
    if (s_pres.powered) {
        return PORT_OK;
    }

    err = dp->power_on();
    if (err == PORT_OK) {
        s_pres.powered = true;
    }
    return err;
}

static bool presentation_blink_valid_timing(uint16_t on_ms, uint16_t off_ms)
{
    return on_ms >= DISPLAY_PRESENTATION_BLINK_MIN_MS &&
           on_ms <= DISPLAY_PRESENTATION_BLINK_MAX_MS &&
           off_ms >= DISPLAY_PRESENTATION_BLINK_MIN_MS &&
           off_ms <= DISPLAY_PRESENTATION_BLINK_MAX_MS;
}

static bool presentation_pattern_valid_phase(uint16_t duration_ms)
{
    return duration_ms >= DISPLAY_PRESENTATION_BLINK_MIN_MS &&
           duration_ms <= DISPLAY_PRESENTATION_BLINK_MAX_MS;
}

static display_pattern_slot_t *presentation_find_pattern(display_icon_t icon)
{
    for (size_t i = 0u; i < DISPLAY_PRESENTATION_MAX_PATTERNS; i++) {
        if (s_pres.patterns[i].active && s_pres.patterns[i].icon == icon) {
            return &s_pres.patterns[i];
        }
    }
    return NULL;
}

static display_pattern_slot_t *presentation_alloc_pattern(void)
{
    for (size_t i = 0u; i < DISPLAY_PRESENTATION_MAX_PATTERNS; i++) {
        if (!s_pres.patterns[i].active) {
            return &s_pres.patterns[i];
        }
    }
    return NULL;
}

static bool presentation_icon_has_pattern(display_icon_t icon)
{
    return presentation_find_pattern(icon) != NULL;
}

static bool presentation_pattern_visible(const display_pattern_slot_t *slot)
{
    return slot->active && slot->phase_visible;
}

static display_blink_slot_t *presentation_find_blink(display_icon_t icon)
{
    for (size_t i = 0u; i < DISPLAY_PRESENTATION_MAX_BLINKS; i++) {
        if (s_pres.blinks[i].active && s_pres.blinks[i].icon == icon) {
            return &s_pres.blinks[i];
        }
    }
    return NULL;
}

static display_blink_slot_t *presentation_alloc_blink(void)
{
    for (size_t i = 0u; i < DISPLAY_PRESENTATION_MAX_BLINKS; i++) {
        if (!s_pres.blinks[i].active) {
            return &s_pres.blinks[i];
        }
    }
    return NULL;
}

static uint32_t presentation_effective_icon_mask(void)
{
    uint32_t mask = s_pres.steady_icons;

    if (s_pres.ota_active) {
        mask &= DISPLAY_GLYPH_ICON_MASK(DISPLAY_ICON_WIFI);
    }

    for (size_t i = 0u; i < DISPLAY_PRESENTATION_MAX_BLINKS; i++) {
        const display_blink_slot_t *slot = &s_pres.blinks[i];

        if (!slot->active || presentation_icon_has_pattern(slot->icon)) {
            continue;
        }

        if (s_pres.ota_active && slot->icon != DISPLAY_ICON_WIFI) {
            continue;
        }

        if (slot->phase_on) {
            mask |= DISPLAY_GLYPH_ICON_MASK(slot->icon);
        } else {
            mask &= ~DISPLAY_GLYPH_ICON_MASK(slot->icon);
        }
    }

    for (size_t i = 0u; i < DISPLAY_PRESENTATION_MAX_PATTERNS; i++) {
        const display_pattern_slot_t *slot = &s_pres.patterns[i];

        if (!slot->active) {
            continue;
        }

        if (s_pres.ota_active && slot->icon != DISPLAY_ICON_WIFI) {
            continue;
        }

        if (presentation_pattern_visible(slot)) {
            mask |= DISPLAY_GLYPH_ICON_MASK(slot->icon);
        } else {
            mask &= ~DISPLAY_GLYPH_ICON_MASK(slot->icon);
        }
    }

    return mask;
}

static uint32_t presentation_min_wake(uint32_t a, uint32_t b)
{
    if (a == DISPLAY_PRESENTATION_TICK_IDLE) {
        return b;
    }
    if (b == DISPLAY_PRESENTATION_TICK_IDLE) {
        return a;
    }
    return (a < b) ? a : b;
}

void display_presentation_reset(void)
{
    memset(&s_pres, 0, sizeof(s_pres));
    s_pres.brightness = 4u;
}

void display_presentation_note_expander_reset(void)
{
    s_pres.powered = false;
}

port_err_t display_presentation_set_digits(uint16_t value)
{
    if (value > 999u) {
        return PORT_ERR_INVALID_ARG;
    }

    s_pres.digits = value;
    s_pres.digits_valid = true;
    s_pres.digits_dash = false;
    s_pres.digits_underflow = false;
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_set_digits_dash(void)
{
    s_pres.digits_valid = true;
    s_pres.digits_dash = true;
    s_pres.digits_underflow = false;
    s_pres.digits = 0u;
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_set_digits_underflow(void)
{
    s_pres.digits_valid = true;
    s_pres.digits_dash = false;
    s_pres.digits_underflow = true;
    s_pres.digits = 0u;
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_clear_digits(void)
{
    s_pres.digits_valid = false;
    s_pres.digits_dash = false;
    s_pres.digits_underflow = false;
    s_pres.digits = 0u;
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_set_unit(display_unit_t unit)
{
    s_pres.unit = unit;
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_icon_set(display_icon_t icon, bool on)
{
    if (icon >= DISPLAY_ICON_COUNT) {
        return PORT_ERR_INVALID_ARG;
    }

    if (on) {
        s_pres.steady_icons |= DISPLAY_GLYPH_ICON_MASK(icon);
    } else {
        s_pres.steady_icons &= ~DISPLAY_GLYPH_ICON_MASK(icon);
    }
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_set_brightness(uint8_t level)
{
    const display_port_t *dp = display_port_get();
    port_err_t err;

    if (level < 1u || level > 4u) {
        return PORT_ERR_INVALID_ARG;
    }

    s_pres.brightness = level;
    if (dp == NULL || dp->set_brightness == NULL) {
        return PORT_ERR_IO;
    }

    err = presentation_ensure_power();
    if (err != PORT_OK) {
        return err;
    }

    return dp->set_brightness(level);
}

port_err_t display_presentation_icon_blink(display_icon_t icon,
                                         uint16_t on_ms,
                                         uint16_t off_ms)
{
    display_blink_slot_t *slot;

    if (icon >= DISPLAY_ICON_COUNT) {
        return PORT_ERR_INVALID_ARG;
    }
    if (!presentation_blink_valid_timing(on_ms, off_ms)) {
        return PORT_ERR_INVALID_ARG;
    }

    slot = presentation_find_blink(icon);
    if (slot == NULL) {
        slot = presentation_alloc_blink();
        if (slot == NULL) {
            return PORT_ERR_BUSY;
        }
    }

    (void)display_presentation_icon_pattern_stop(icon);

    slot->active = true;
    slot->icon = icon;
    slot->on_ms = on_ms;
    slot->off_ms = off_ms;
    slot->phase_on = true;
    slot->phase_until_ms = s_pres.last_now_ms + on_ms;
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_icon_blink_stop(display_icon_t icon)
{
    display_blink_slot_t *slot = presentation_find_blink(icon);

    if (icon >= DISPLAY_ICON_COUNT) {
        return PORT_ERR_INVALID_ARG;
    }
    if (slot != NULL) {
        slot->active = false;
    }
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_icon_pattern(display_icon_t icon,
                                             const display_pattern_phase_t *phases,
                                             uint8_t phase_count,
                                             bool loop)
{
    display_pattern_slot_t *slot;
    uint8_t i;

    if (icon >= DISPLAY_ICON_COUNT || phases == NULL) {
        return PORT_ERR_INVALID_ARG;
    }
    if (phase_count == 0u ||
        phase_count > DISPLAY_PRESENTATION_MAX_PATTERN_PHASES) {
        return PORT_ERR_INVALID_ARG;
    }

    for (i = 0u; i < phase_count; i++) {
        if (!presentation_pattern_valid_phase(phases[i].duration_ms)) {
            return PORT_ERR_INVALID_ARG;
        }
    }

    slot = presentation_find_pattern(icon);
    if (slot == NULL) {
        slot = presentation_alloc_pattern();
        if (slot == NULL) {
            return PORT_ERR_BUSY;
        }
    }

    (void)display_presentation_icon_blink_stop(icon);

    slot->active = true;
    slot->icon = icon;
    slot->loop = loop;
    slot->phase_count = phase_count;
    for (i = 0u; i < phase_count; i++) {
        slot->phases[i] = phases[i];
    }
    slot->phase_index = 0u;
    slot->phase_visible = phases[0].visible;
    slot->phase_until_ms = s_pres.last_now_ms + phases[0].duration_ms;
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_icon_pattern_stop(display_icon_t icon)
{
    display_pattern_slot_t *slot = presentation_find_pattern(icon);

    if (icon >= DISPLAY_ICON_COUNT) {
        return PORT_ERR_INVALID_ARG;
    }
    if (slot != NULL) {
        slot->active = false;
    }
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_play_animation(const display_animation_t *anim,
                                               bool loop)
{
    if (anim == NULL || anim->frames == NULL || anim->frame_count == 0u ||
        anim->frame_ms < DISPLAY_PRESENTATION_BLINK_MIN_MS) {
        return PORT_ERR_INVALID_ARG;
    }

    s_pres.anim = anim;
    s_pres.anim_loop = loop;
    s_pres.anim_frame = 0u;
    s_pres.anim_next_ms = s_pres.last_now_ms + anim->frame_ms;
    s_pres.anim_running = true;
    return PORT_OK;
}

port_err_t display_presentation_play_builtin(display_builtin_anim_t id, bool loop)
{
    const display_animation_t *anim = display_anim_builtin_get(id);

    if (anim == NULL) {
        return PORT_ERR_INVALID_ARG;
    }
    return display_presentation_play_animation(anim, loop);
}

port_err_t display_presentation_stop_animation(void)
{
    s_pres.anim_running = false;
    s_pres.anim = NULL;
    return PORT_OK;
}

static uint16_t presentation_ota_blink_ms(display_ota_phase_t phase)
{
    switch (phase) {
    case DISPLAY_OTA_PHASE_CONNECTING:
        return DISPLAY_OTA_CONNECTING_BLINK_MS;
    case DISPLAY_OTA_PHASE_VERIFYING:
        return DISPLAY_OTA_VERIFYING_BLINK_MS;
    default:
        return 0u;
    }
}

static void presentation_compose_ota_grids(uint8_t grids[TM1637_GRID_COUNT])
{
    uint32_t icons = presentation_effective_icon_mask();
    uint8_t icon_grids[TM1637_GRID_COUNT];

    switch (s_pres.ota_phase) {
    case DISPLAY_OTA_PHASE_CONNECTING:
        display_glyph_ota_bar(0u, s_pres.ota_blink_on, grids);
        break;
    case DISPLAY_OTA_PHASE_DOWNLOADING: {
        uint8_t filled = display_glyph_ota_filled_from_pct(s_pres.ota_pct);

        display_glyph_ota_bar(filled, false, grids);
        break;
    }
    case DISPLAY_OTA_PHASE_VERIFYING:
        display_glyph_ota_bar(DISPLAY_GLYPH_OTA_PATH_LEN, s_pres.ota_blink_on, grids);
        break;
    default:
        grids[0] = 0x00u;
        grids[1] = 0x00u;
        grids[2] = 0x00u;
        grids[3] = 0x00u;
        grids[4] = 0x00u;
        break;
    }

    display_compose_grids(false, 0u, DISPLAY_UNIT_NONE, icons, icon_grids);
    grids[3] = icon_grids[3];
    grids[4] = icon_grids[4];
}

port_err_t display_presentation_ota_show(display_ota_phase_t phase, uint8_t pct)
{
    uint16_t blink_ms = presentation_ota_blink_ms(phase);

    s_pres.ota_active = true;
    s_pres.ota_phase = phase;
    s_pres.ota_pct = pct;
    s_pres.ota_blink_on = true;
    if (blink_ms > 0u) {
        s_pres.ota_blink_until_ms = s_pres.last_now_ms + blink_ms;
    } else {
        s_pres.ota_blink_until_ms = DISPLAY_PRESENTATION_TICK_IDLE;
    }
    presentation_mark_scene_dirty();
    return PORT_OK;
}

port_err_t display_presentation_ota_stop(void)
{
    s_pres.ota_active = false;
    s_pres.ota_blink_until_ms = DISPLAY_PRESENTATION_TICK_IDLE;
    presentation_mark_scene_dirty();
    return PORT_OK;
}

bool display_presentation_ota_active(void)
{
    return s_pres.ota_active;
}

port_err_t display_presentation_power_on(void)
{
    return presentation_ensure_power();
}

port_err_t display_presentation_power_off(void)
{
    const display_port_t *dp = display_port_get();
    port_err_t err;

    if (dp == NULL) {
        return PORT_ERR_IO;
    }

    err = dp->power_off();
    if (err == PORT_OK) {
        s_pres.powered = false;
    }
    return err;
}

port_err_t display_presentation_refresh(void)
{
    const display_port_t *dp = display_port_get();
    uint8_t grids[TM1637_GRID_COUNT];
    port_err_t err;

    if (dp == NULL || dp->show_grids == NULL) {
        return PORT_ERR_IO;
    }

    err = presentation_ensure_power();
    if (err != PORT_OK) {
        return err;
    }

    if (dp->set_brightness != NULL) {
        err = dp->set_brightness(s_pres.brightness);
        if (err != PORT_OK) {
            return err;
        }
    }

    if (s_pres.ota_active) {
        presentation_compose_ota_grids(grids);
    } else if (s_pres.anim_running && s_pres.anim != NULL) {
        memcpy(grids, s_pres.anim->frames[s_pres.anim_frame], sizeof(grids));
    } else if (s_pres.digits_valid && s_pres.digits_dash) {
        uint32_t icons = presentation_effective_icon_mask();

        grids[0] = 0x40u;
        grids[1] = 0x40u;
        grids[2] = 0x40u;
        display_compose_grids(false,
                              0u,
                              s_pres.unit,
                              icons,
                              grids);
        grids[0] = 0x40u;
        grids[1] = 0x40u;
        grids[2] = 0x40u;
    } else if (s_pres.digits_valid && s_pres.digits_underflow) {
        uint32_t icons = presentation_effective_icon_mask();

        grids[0] = 0x40u;
        grids[1] = 0x00u;
        grids[2] = 0x00u;
        display_compose_grids(false,
                              0u,
                              s_pres.unit,
                              icons,
                              grids);
        grids[0] = 0x40u;
        grids[1] = 0x00u;
        grids[2] = 0x00u;
    } else {
        uint32_t icons = presentation_effective_icon_mask();
        display_compose_grids(s_pres.digits_valid,
                              s_pres.digits,
                              s_pres.unit,
                              icons,
                              grids);
    }

    if (dp->try_show_grids != NULL) {
        err = dp->try_show_grids(grids);
    } else {
        err = dp->show_grids(grids);
    }
    if (err == PORT_OK) {
        s_pres.scene_dirty = false;
    }
    return err;
}

uint32_t display_presentation_tick(uint32_t now_ms)
{
    bool needs_refresh = false;
    uint32_t next_wake = DISPLAY_PRESENTATION_TICK_IDLE;

    s_pres.last_now_ms = now_ms;

    if (s_pres.ota_active) {
        uint16_t blink_ms = presentation_ota_blink_ms(s_pres.ota_phase);

        if (blink_ms > 0u) {
            if (now_ms >= s_pres.ota_blink_until_ms) {
                s_pres.ota_blink_on = !s_pres.ota_blink_on;
                s_pres.ota_blink_until_ms = now_ms + blink_ms;
                needs_refresh = true;
            }

            if (s_pres.ota_blink_until_ms > now_ms) {
                uint32_t remain = s_pres.ota_blink_until_ms - now_ms;

                next_wake = presentation_min_wake(next_wake, remain);
            }
        }
    }

    if (s_pres.anim_running && s_pres.anim != NULL) {
        if (now_ms >= s_pres.anim_next_ms) {
            s_pres.anim_frame++;
            if (s_pres.anim_frame >= s_pres.anim->frame_count) {
                if (s_pres.anim_loop) {
                    s_pres.anim_frame = 0u;
                } else {
                    s_pres.anim_frame =
                        (uint8_t)(s_pres.anim->frame_count - 1u);
                    s_pres.anim_running = false;
                }
            }
            if (s_pres.anim_running) {
                s_pres.anim_next_ms = now_ms + s_pres.anim->frame_ms;
                next_wake = presentation_min_wake(next_wake, s_pres.anim->frame_ms);
            }
            needs_refresh = true;
        } else {
            uint32_t remain = s_pres.anim_next_ms - now_ms;
            next_wake = presentation_min_wake(next_wake, remain);
        }
    }

    for (size_t i = 0u; i < DISPLAY_PRESENTATION_MAX_BLINKS; i++) {
        display_blink_slot_t *slot = &s_pres.blinks[i];
        bool was_on;

        if (!slot->active || s_pres.anim_running ||
            (s_pres.ota_active && slot->icon != DISPLAY_ICON_WIFI)) {
            continue;
        }

        was_on = slot->phase_on;
        if (now_ms >= slot->phase_until_ms) {
            slot->phase_on = !slot->phase_on;
            slot->phase_until_ms = now_ms +
                (slot->phase_on ? slot->on_ms : slot->off_ms);
            if (was_on != slot->phase_on) {
                needs_refresh = true;
            }
        }

        if (slot->phase_until_ms > now_ms) {
            uint32_t remain = slot->phase_until_ms - now_ms;
            next_wake = presentation_min_wake(next_wake, remain);
        }
    }

    for (size_t i = 0u; i < DISPLAY_PRESENTATION_MAX_PATTERNS; i++) {
        display_pattern_slot_t *slot = &s_pres.patterns[i];
        bool was_visible;

        if (!slot->active || s_pres.anim_running ||
            (s_pres.ota_active && slot->icon != DISPLAY_ICON_WIFI)) {
            continue;
        }

        while (slot->active && now_ms >= slot->phase_until_ms) {
            uint8_t next_index = (uint8_t)(slot->phase_index + 1u);
            uint32_t transition_at = slot->phase_until_ms;

            was_visible = slot->phase_visible;

            if (next_index >= slot->phase_count) {
                if (slot->loop) {
                    next_index = 0u;
                    transition_at = now_ms;
                } else {
                    slot->active = false;
                    needs_refresh = true;
                    break;
                }
            }

            slot->phase_index = next_index;
            slot->phase_visible = slot->phases[next_index].visible;
            slot->phase_until_ms =
                transition_at + slot->phases[next_index].duration_ms;
            if (was_visible != slot->phase_visible) {
                needs_refresh = true;
            }
        }

        if (slot->active && slot->phase_until_ms > now_ms) {
            uint32_t remain = slot->phase_until_ms - now_ms;
            next_wake = presentation_min_wake(next_wake, remain);
        }
    }

    if (s_pres.scene_dirty) {
        needs_refresh = true;
    }

    if (needs_refresh) {
        (void)display_presentation_refresh();
    }

    return next_wake;
}

bool display_presentation_parse_icon(const char *name, display_icon_t *out)
{
    if (name == NULL) {
        return false;
    }

    for (display_icon_t icon = 0; icon < DISPLAY_ICON_COUNT; icon++) {
        if (strcmp(name, s_icon_names[icon]) == 0) {
            if (out != NULL) {
                *out = icon;
            }
            return true;
        }
    }
    return false;
}

bool display_presentation_parse_builtin_anim(const char *name,
                                             display_builtin_anim_t *out)
{
    if (name == NULL) {
        return false;
    }
    if (strcmp(name, "ota") == 0) {
        if (out != NULL) {
            *out = DISPLAY_ANIM_OTA;
        }
        return true;
    }
    return false;
}
