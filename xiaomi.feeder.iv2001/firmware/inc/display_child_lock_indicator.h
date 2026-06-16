/*
 * Child lock pictograph policy — spec/30-processes/display-presentation.md § Lock indicator
 */

#ifndef DISPLAY_CHILD_LOCK_INDICATOR_H
#define DISPLAY_CHILD_LOCK_INDICATOR_H

#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_CHILD_LOCK_BLOCKED_MS      1000u
#define DISPLAY_CHILD_LOCK_BLINK_ON_MS    200u
#define DISPLAY_CHILD_LOCK_BLINK_OFF_MS   200u

void display_child_lock_indicator_blocked_feedback(bool restore_steady_icon,
                                                   uint32_t now_ms);
void display_child_lock_indicator_cancel(void);
bool display_child_lock_indicator_poll(uint32_t now_ms);
bool display_child_lock_indicator_feedback_active(void);
void display_child_lock_indicator_test_reset(void);

#endif /* DISPLAY_CHILD_LOCK_INDICATOR_H */
