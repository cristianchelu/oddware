/*
 * Hopper level debounce — spec/30-processes/hopper-sensing.md
 */

#include <stdio.h>

#include "hopper_input.h"

static const hopper_ir_port_t *s_port;
static hopper_level_t s_level = HOPPER_LEVEL_NORMAL;
static uint8_t s_clear_streak;
static uint8_t s_blocked_streak;
static bool s_debounce_active;
static bool s_force_sample;
static uint32_t s_next_sample_ms;
static uint32_t s_last_background_ms;
static hopper_level_transition_t s_transition_queue[HOPPER_INPUT_TRANSITION_QUEUE_DEPTH];
static uint8_t s_transition_head;
static uint8_t s_transition_tail;

static void hopper_input_enqueue_transition(hopper_level_t level, uint32_t at_ms)
{
    uint8_t next =
        (uint8_t)((s_transition_tail + 1u) % HOPPER_INPUT_TRANSITION_QUEUE_DEPTH);

    if (next == s_transition_head) {
        /* Queue full — drop oldest unconsumed transition (MQTT publish deferred). */
        return;
    }

    s_transition_queue[s_transition_tail].level = level;
    s_transition_queue[s_transition_tail].at_ms = at_ms;
    s_transition_tail = next;
}

static void hopper_input_set_level(hopper_level_t level, uint32_t at_ms)
{
    if (level == s_level) {
        return;
    }

    s_level = level;
    hopper_input_enqueue_transition(level, at_ms);
    printf("[hopper] level %s\r\n", level == HOPPER_LEVEL_LOW ? "low" : "normal");
}

static bool hopper_input_should_sample(uint32_t now_ms)
{
    if (s_force_sample) {
        return true;
    }

    if (s_debounce_active && now_ms >= s_next_sample_ms) {
        return true;
    }

    if (!s_debounce_active) {
        if (s_last_background_ms == 0u) {
            s_last_background_ms = now_ms;
            return false;
        }

        if ((now_ms - s_last_background_ms) >= HOPPER_INPUT_BACKGROUND_INTERVAL_MS) {
            return true;
        }
    }

    return false;
}

static void hopper_input_apply_reading(bool beam_blocked, uint32_t now_ms)
{
    if (beam_blocked) {
        s_clear_streak = 0u;

        if (s_level == HOPPER_LEVEL_LOW) {
            s_blocked_streak++;
            if (s_blocked_streak >= HOPPER_INPUT_NORMAL_STREAK_REQUIRED) {
                hopper_input_set_level(HOPPER_LEVEL_NORMAL, now_ms);
                s_blocked_streak = 0u;
                s_debounce_active = false;
            } else {
                s_debounce_active = true;
                s_next_sample_ms = now_ms + HOPPER_INPUT_DEBOUNCE_INTERVAL_MS;
            }
        } else {
            s_debounce_active = false;
        }

        return;
    }

    s_blocked_streak = 0u;

    if (s_level == HOPPER_LEVEL_LOW) {
        return;
    }

    s_clear_streak++;
    if (s_clear_streak >= HOPPER_INPUT_LOW_STREAK_REQUIRED) {
        hopper_input_set_level(HOPPER_LEVEL_LOW, now_ms);
        s_clear_streak = 0u;
        s_debounce_active = false;
    } else {
        s_debounce_active = true;
        s_next_sample_ms = now_ms + HOPPER_INPUT_DEBOUNCE_INTERVAL_MS;
    }
}

static void hopper_input_sample(uint32_t now_ms, bool background)
{
    bool beam_blocked;
    port_err_t err;

    if (s_port == NULL || s_port->sense == NULL) {
        return;
    }

    err = s_port->sense(&beam_blocked);
    s_force_sample = false;

    if (err != PORT_OK) {
        return;
    }

    if (background) {
        s_last_background_ms = now_ms;
    }

    hopper_input_apply_reading(beam_blocked, now_ms);
}

void hopper_input_init(const hopper_ir_port_t *port)
{
    s_port = port;
    hopper_input_reset();
}

void hopper_input_reset(void)
{
    s_level = HOPPER_LEVEL_NORMAL;
    s_clear_streak = 0u;
    s_blocked_streak = 0u;
    s_debounce_active = false;
    s_force_sample = false;
    s_next_sample_ms = 0u;
    s_last_background_ms = 0u;
    s_transition_head = 0u;
    s_transition_tail = 0u;
}

void hopper_input_notify_dispense_complete(void)
{
    s_clear_streak = 0u;
    s_force_sample = true;
}

void hopper_input_poll(uint32_t now_ms)
{
    bool background;

    if (!hopper_input_should_sample(now_ms)) {
        return;
    }

    background = !s_force_sample && !s_debounce_active;
    hopper_input_sample(now_ms, background);
}

hopper_level_t hopper_input_get_level(void)
{
    return s_level;
}

bool hopper_input_almost_empty(void)
{
    return s_level == HOPPER_LEVEL_LOW;
}

bool hopper_input_pop_transition(hopper_level_transition_t *tr)
{
    if (tr == NULL || s_transition_head == s_transition_tail) {
        return false;
    }

    *tr = s_transition_queue[s_transition_head];
    s_transition_head =
        (uint8_t)((s_transition_head + 1u) % HOPPER_INPUT_TRANSITION_QUEUE_DEPTH);
    return true;
}
