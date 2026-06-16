/*
 * Child lock pictograph policy — spec/30-processes/display-presentation.md § Lock indicator
 */

#include "display_child_lock_indicator.h"

#include "display_glyph.h"
#include "display_presentation.h"

static bool s_feedback_active;
static bool s_restore_steady_icon;
static uint32_t s_feedback_until_ms;

void display_child_lock_indicator_test_reset(void)
{
    s_feedback_active = false;
    s_restore_steady_icon = false;
    s_feedback_until_ms = 0u;
}

bool display_child_lock_indicator_feedback_active(void)
{
    return s_feedback_active;
}

void display_child_lock_indicator_blocked_feedback(bool restore_steady_icon,
                                                   uint32_t now_ms)
{
    (void)display_presentation_stop_animation();
    (void)display_presentation_clear_digits();
    (void)display_presentation_set_unit(DISPLAY_UNIT_NONE);
    (void)display_presentation_icon_set(DISPLAY_ICON_CHILD_LOCK, false);
    (void)display_presentation_icon_blink(DISPLAY_ICON_CHILD_LOCK,
                                         DISPLAY_CHILD_LOCK_BLINK_ON_MS,
                                         DISPLAY_CHILD_LOCK_BLINK_OFF_MS);

    s_restore_steady_icon = restore_steady_icon;
    s_feedback_active = true;
    s_feedback_until_ms = now_ms + DISPLAY_CHILD_LOCK_BLOCKED_MS;
    (void)display_presentation_refresh();
}

bool display_child_lock_indicator_poll(uint32_t now_ms)
{
    if (!s_feedback_active) {
        return false;
    }

    if (now_ms < s_feedback_until_ms) {
        return false;
    }

    (void)display_presentation_icon_blink_stop(DISPLAY_ICON_CHILD_LOCK);
    if (s_restore_steady_icon) {
        (void)display_presentation_icon_set(DISPLAY_ICON_CHILD_LOCK, true);
    }

    s_feedback_active = false;
    s_feedback_until_ms = 0u;
    return true;
}
