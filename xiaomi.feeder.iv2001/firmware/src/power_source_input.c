/*
 * Mains sense debounce — spec/30-processes/power-state-machine.md § Mains sense input
 */

#include <stddef.h>

#include "app_log.h"
#include "power_source_input.h"

static const power_source_port_t *s_port;
static uint32_t s_irq_sample_after_ms;
static bool s_last_mains;
static bool s_confirmed_mains;
static bool s_valid;
static power_source_transition_t s_transition_queue[POWER_SOURCE_INPUT_TRANSITION_QUEUE_DEPTH];
static uint8_t s_transition_head;
static uint8_t s_transition_tail;

static void power_source_input_enqueue_transition(power_source_edge_t edge,
                                                  uint32_t at_ms)
{
    uint8_t next =
        (uint8_t)((s_transition_tail + 1u) % POWER_SOURCE_INPUT_TRANSITION_QUEUE_DEPTH);

    if (next == s_transition_head) {
        return;
    }

    s_transition_queue[s_transition_tail].edge = edge;
    s_transition_queue[s_transition_tail].at_ms = at_ms;
    s_transition_tail = next;
}

static void power_source_input_confirm(bool mains_present, uint32_t at_ms)
{
    power_source_edge_t edge =
        mains_present ? POWER_SOURCE_EDGE_MAINS : POWER_SOURCE_EDGE_BATTERY;

    s_confirmed_mains = mains_present;
    power_source_input_enqueue_transition(edge, at_ms);
    if (edge == POWER_SOURCE_EDGE_MAINS) {
        app_log_info("power", "mains connected");
    } else {
        app_log_info("power", "mains lost");
    }
}

static bool power_source_input_may_sample(uint32_t now_ms, bool bypass_irq_gate)
{
    if (bypass_irq_gate || s_irq_sample_after_ms == 0u) {
        return true;
    }

    return now_ms >= s_irq_sample_after_ms;
}

static void power_source_input_apply_reading(bool mains_present, uint32_t now_ms)
{
    if (mains_present == s_last_mains) {
        if (mains_present != s_confirmed_mains) {
            power_source_input_confirm(mains_present, now_ms);
        }
    }

    s_last_mains = mains_present;
}

static void power_source_input_read(bool bypass_irq_gate, uint32_t now_ms)
{
    bool mains_present;
    port_err_t err;

    if (!power_source_input_may_sample(now_ms, bypass_irq_gate)) {
        return;
    }

    if (s_port == NULL || s_port->read_present == NULL) {
        return;
    }

    err = s_port->read_present(&mains_present);
    if (err != PORT_OK) {
        return;
    }

    s_valid = true;
    power_source_input_apply_reading(mains_present, now_ms);
}

static void power_source_input_boot_seed(void)
{
    bool mains_present;
    port_err_t err;

    if (s_port == NULL || s_port->read_present == NULL) {
        return;
    }

    err = s_port->read_present(&mains_present);
    if (err != PORT_OK) {
        s_valid = false;
        return;
    }

    s_valid = true;
    s_last_mains = mains_present;
    s_confirmed_mains = mains_present;
}

void power_source_input_init(const power_source_port_t *port)
{
    s_port = port;
    power_source_input_reset();
    power_source_input_boot_seed();
}

void power_source_input_reset(void)
{
    s_irq_sample_after_ms = 0u;
    s_last_mains = false;
    s_confirmed_mains = false;
    s_valid = false;
    s_transition_head = 0u;
    s_transition_tail = 0u;
}

void power_source_input_notify_irq(uint32_t now_ms)
{
    uint32_t ready_at = now_ms + POWER_SOURCE_INPUT_DEBOUNCE_MS;

    if (ready_at > s_irq_sample_after_ms) {
        s_irq_sample_after_ms = ready_at;
    }
}

void power_source_input_poll(uint32_t now_ms)
{
    power_source_input_read(false, now_ms);
}

power_source_t power_source_input_get(void)
{
    return s_confirmed_mains ? POWER_SOURCE_MAINS : POWER_SOURCE_BATTERY;
}

bool power_source_input_is_valid(void)
{
    return s_valid;
}

bool power_source_input_pop_transition(power_source_transition_t *tr)
{
    if (tr == NULL || s_transition_head == s_transition_tail) {
        return false;
    }

    *tr = s_transition_queue[s_transition_head];
    s_transition_head =
        (uint8_t)((s_transition_head + 1u) % POWER_SOURCE_INPUT_TRANSITION_QUEUE_DEPTH);
    return true;
}
