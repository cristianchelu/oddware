/*
 * Button gesture classification — spec/30-processes/button-handling.md
 */

#ifndef BUTTON_GESTURE_H
#define BUTTON_GESTURE_H

#include <stdbool.h>
#include <stdint.h>

#include "button_input.h"

typedef enum {
    BUTTON_GESTURE_SHORT = 0,
    BUTTON_GESTURE_LONG,
    BUTTON_GESTURE_CHILD_LOCK_TOGGLE,
} button_gesture_kind_t;

typedef struct {
    button_id_t id;
    button_gesture_kind_t kind;
    uint32_t at_ms;
} button_gesture_event_t;

#define BUTTON_GESTURE_SHORT_MAX_MS        1000u
#define BUTTON_GESTURE_DISPENSE_LONG_MS    2000u
#define BUTTON_GESTURE_POWER_LONG_MS       3000u
#define BUTTON_GESTURE_RESET_LONG_MS       7000u
#define BUTTON_GESTURE_CHILD_LOCK_MS       3000u
#define BUTTON_GESTURE_EVENT_QUEUE_DEPTH   4u

void button_gesture_reset(void);
void button_gesture_on_transition(const button_transition_t *tr);
void button_gesture_step(uint32_t now_ms);
bool button_gesture_pop(button_gesture_event_t *ev);

#endif /* BUTTON_GESTURE_H */
