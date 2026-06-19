/*
 * Hopper published level compositor — spec/30-processes/hopper-sensing.md § hopper_level
 */

#include "hopper_level.h"

#include <stddef.h>

#include "hopper_input.h"

static hopper_level_state_t s_published = HOPPER_LEVEL_STATE_NORMAL;
static bool s_empty_latched;
static hopper_level_state_transition_t s_transition_queue[HOPPER_LEVEL_TRANSITION_QUEUE_DEPTH];
static uint8_t s_transition_head;
static uint8_t s_transition_tail;

static hopper_level_state_t hopper_level_compute(void)
{
    if (s_empty_latched) {
        return HOPPER_LEVEL_STATE_EMPTY;
    }

    if (hopper_input_almost_empty()) {
        return HOPPER_LEVEL_STATE_LOW;
    }

    return HOPPER_LEVEL_STATE_NORMAL;
}

static void hopper_level_enqueue_transition(hopper_level_state_t level, uint32_t at_ms)
{
    uint8_t next =
        (uint8_t)((s_transition_tail + 1u) % HOPPER_LEVEL_TRANSITION_QUEUE_DEPTH);

    if (next == s_transition_head) {
        return;
    }

    s_transition_queue[s_transition_tail].level = level;
    s_transition_queue[s_transition_tail].at_ms = at_ms;
    s_transition_tail = next;
}

static void hopper_level_set_published(hopper_level_state_t level, uint32_t at_ms)
{
    if (level == s_published) {
        return;
    }

    s_published = level;
    hopper_level_enqueue_transition(level, at_ms);
}

static void hopper_level_sync_published(uint32_t at_ms)
{
    hopper_level_set_published(hopper_level_compute(), at_ms);
}

static bool hopper_level_should_latch_empty(dispense_outcome_t outcome,
                                            int32_t raw_delta,
                                            bool measured)
{
    if (outcome == DISPENSE_OUTCOME_UNDERFILL) {
        return true;
    }

    return outcome == DISPENSE_OUTCOME_SUCCESS && measured && raw_delta <= 0 &&
           hopper_input_almost_empty();
}

void hopper_level_init(void)
{
    hopper_level_reset();
}

void hopper_level_reset(void)
{
    s_published = HOPPER_LEVEL_STATE_NORMAL;
    s_empty_latched = false;
    s_transition_head = 0u;
    s_transition_tail = 0u;
}

void hopper_level_poll(void)
{
    hopper_level_transition_t ir_tr;

    while (hopper_input_pop_transition(&ir_tr)) {
        if (ir_tr.level == HOPPER_LEVEL_NORMAL && s_empty_latched) {
            s_empty_latched = false;
        }

        hopper_level_sync_published(ir_tr.at_ms);
    }
}

dispense_outcome_t hopper_level_on_dispense_finished(dispense_outcome_t outcome,
                                                     int32_t raw_delta,
                                                     bool measured,
                                                     uint32_t now_ms)
{
    if (outcome == DISPENSE_OUTCOME_SUCCESS && measured && raw_delta > 0) {
        s_empty_latched = false;
    }

    if (hopper_level_should_latch_empty(outcome, raw_delta, measured)) {
        s_empty_latched = true;
    }

    hopper_level_sync_published(now_ms);

    if (outcome == DISPENSE_OUTCOME_SUCCESS && s_published == HOPPER_LEVEL_STATE_EMPTY) {
        return DISPENSE_OUTCOME_EMPTY_HOPPER;
    }

    return outcome;
}

void hopper_level_notify_dispense_complete(void)
{
    hopper_input_notify_dispense_complete();
}

hopper_level_state_t hopper_level_get(void)
{
    return s_published;
}

bool hopper_level_pop_transition(hopper_level_state_transition_t *tr)
{
    if (tr == NULL || s_transition_head == s_transition_tail) {
        return false;
    }

    *tr = s_transition_queue[s_transition_head];
    s_transition_head =
        (uint8_t)((s_transition_head + 1u) % HOPPER_LEVEL_TRANSITION_QUEUE_DEPTH);
    return true;
}
