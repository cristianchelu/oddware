/*
 * Built-in display animations — spec/30-processes/display-presentation.md
 */

#include <stddef.h>

#include "display_anim_builtin.h"
#include "display_glyph.h"

static const uint8_t s_ota_frames[][TM1637_GRID_COUNT] = {
    { 0x41u, 0x40u, 0x40u, 0x00u, 0x00u },
    { 0x40u, 0x41u, 0x40u, 0x00u, 0x00u },
    { 0x40u, 0x40u, 0x41u, 0x00u, 0x00u },
    { 0x40u, 0x40u, 0x42u, 0x00u, 0x00u },
    { 0x40u, 0x40u, 0x44u, 0x00u, 0x00u },
    { 0x40u, 0x40u, 0x48u, 0x00u, 0x00u },
    { 0x40u, 0x48u, 0x40u, 0x00u, 0x00u },
    { 0x48u, 0x40u, 0x40u, 0x00u, 0x00u },
    { 0x50u, 0x40u, 0x40u, 0x00u, 0x00u },
    { 0x60u, 0x40u, 0x40u, 0x00u, 0x00u },
};

static const display_animation_t s_builtin_anims[DISPLAY_BUILTIN_ANIM_COUNT] = {
    [DISPLAY_ANIM_OTA] = {
        .frames = s_ota_frames,
        .frame_count = (uint8_t)(sizeof(s_ota_frames) / sizeof(s_ota_frames[0])),
        .frame_ms = 150u,
    },
};

const display_animation_t *display_anim_builtin_get(display_builtin_anim_t id)
{
    if (id >= DISPLAY_BUILTIN_ANIM_COUNT) {
        return NULL;
    }
    return &s_builtin_anims[id];
}
