/*
 * Button gesture classification — spec/30-processes/button-handling.md
 */

#include <stddef.h>

#include "button_gesture.h"

typedef enum {
    BTN_GESTURE_STATE_IDLE = 0,
    BTN_GESTURE_STATE_HELD,
    BTN_GESTURE_STATE_LONG_FIRED,
} btn_gesture_state_t;

typedef struct {
    btn_gesture_state_t state;
    uint32_t down_ms;
    bool long_fired;
} btn_channel_gesture_t;

static btn_channel_gesture_t s_channels[3];
static bool s_down[3];
static button_gesture_event_t s_event_queue[BUTTON_GESTURE_EVENT_QUEUE_DEPTH];
static uint8_t s_event_head;
static uint8_t s_event_tail;
static uint32_t s_combo_start_ms;
static bool s_combo_active;
static bool s_combo_fired;

static uint32_t button_gesture_long_ms(button_id_t id)
{
    switch (id) {
    case BUTTON_ID_POWER:
        return BUTTON_GESTURE_POWER_LONG_MS;
    case BUTTON_ID_RESET:
        return BUTTON_GESTURE_RESET_LONG_MS;
    case BUTTON_ID_DISPENSE:
        return BUTTON_GESTURE_DISPENSE_LONG_MS;
    default:
        return UINT32_MAX;
    }
}

static void button_gesture_enqueue(button_id_t id,
                                 button_gesture_kind_t kind,
                                 uint32_t at_ms)
{
    uint8_t next = (uint8_t)((s_event_tail + 1u) % BUTTON_GESTURE_EVENT_QUEUE_DEPTH);

    if (next == s_event_head) {
        return;
    }

    s_event_queue[s_event_tail].id = id;
    s_event_queue[s_event_tail].kind = kind;
    s_event_queue[s_event_tail].at_ms = at_ms;
    s_event_tail = next;
}

static void button_gesture_update_combo(uint32_t at_ms)
{
    if (s_down[BUTTON_ID_RESET] && s_down[BUTTON_ID_DISPENSE]) {
        if (!s_combo_active) {
            s_combo_active = true;
            s_combo_start_ms = at_ms;
            s_combo_fired = false;
        }
        return;
    }

    s_combo_active = false;
    s_combo_fired = false;
}

void button_gesture_reset(void)
{
    size_t i;

    s_event_head = 0u;
    s_event_tail = 0u;
    s_combo_active = false;
    s_combo_fired = false;
    s_combo_start_ms = 0u;

    for (i = 0u; i < (sizeof(s_channels) / sizeof(s_channels[0])); i++) {
        s_channels[i].state = BTN_GESTURE_STATE_IDLE;
        s_channels[i].down_ms = 0u;
        s_channels[i].long_fired = false;
        s_down[i] = false;
    }
}

void button_gesture_on_transition(const button_transition_t *tr)
{
    btn_channel_gesture_t *ch;

    if (tr == NULL || tr->id > BUTTON_ID_DISPENSE) {
        return;
    }

    ch = &s_channels[tr->id];

    if (tr->edge == BUTTON_EDGE_DOWN) {
        s_down[tr->id] = true;
        ch->state = BTN_GESTURE_STATE_HELD;
        ch->down_ms = tr->at_ms;
        ch->long_fired = false;
        button_gesture_update_combo(tr->at_ms);
        return;
    }

    s_down[tr->id] = false;
    button_gesture_update_combo(tr->at_ms);

    if (ch->state == BTN_GESTURE_STATE_HELD && !ch->long_fired) {
        button_gesture_enqueue(tr->id, BUTTON_GESTURE_SHORT, tr->at_ms);
    }

    ch->state = BTN_GESTURE_STATE_IDLE;
    ch->long_fired = false;
}

void button_gesture_step(uint32_t now_ms)
{
    size_t i;

    if (s_combo_active && !s_combo_fired && s_down[BUTTON_ID_RESET] &&
        s_down[BUTTON_ID_DISPENSE] &&
        now_ms >= (s_combo_start_ms + BUTTON_GESTURE_CHILD_LOCK_MS)) {
        button_gesture_enqueue(BUTTON_ID_RESET,
                               BUTTON_GESTURE_CHILD_LOCK_TOGGLE,
                               now_ms);
        s_combo_fired = true;
    }

    for (i = 0u; i < (sizeof(s_channels) / sizeof(s_channels[0])); i++) {
        btn_channel_gesture_t *ch = &s_channels[i];

        if (ch->state != BTN_GESTURE_STATE_HELD || ch->long_fired) {
            continue;
        }

        if (now_ms >= (ch->down_ms + button_gesture_long_ms((button_id_t)i))) {
            button_gesture_enqueue((button_id_t)i, BUTTON_GESTURE_LONG, now_ms);
            ch->long_fired = true;
            ch->state = BTN_GESTURE_STATE_LONG_FIRED;
        }
    }
}

bool button_gesture_pop(button_gesture_event_t *ev)
{
    if (ev == NULL || s_event_head == s_event_tail) {
        return false;
    }

    *ev = s_event_queue[s_event_head];
    s_event_head = (uint8_t)((s_event_head + 1u) % BUTTON_GESTURE_EVENT_QUEUE_DEPTH);
    return true;
}
