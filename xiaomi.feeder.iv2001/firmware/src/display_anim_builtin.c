/*
 * Built-in display animations — spec/30-processes/display-presentation.md
 */

#include <stddef.h>

#include "display_anim_builtin.h"

static const uint8_t s_ota_frames[][TM1637_GRID_COUNT] = {
    { 0x01u, 0x04u, 0x10u, 0x00u, 0x00u },
    { 0x02u, 0x08u, 0x20u, 0x00u, 0x00u },
    { 0x04u, 0x10u, 0x01u, 0x00u, 0x00u },
    { 0x08u, 0x20u, 0x02u, 0x00u, 0x00u },
    { 0x10u, 0x01u, 0x04u, 0x00u, 0x00u },
    { 0x20u, 0x02u, 0x08u, 0x00u, 0x00u },
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
