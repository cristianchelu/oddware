/*
 * Button debounce and transition detection — spec/30-processes/button-handling.md
 */

#include <stddef.h>

#include "button_input.h"

typedef struct {
    button_id_t id;
    bool irq_backed;
    bool (*is_pressed)(const button_sample_t *sample);
    bool last;
    bool confirmed;
} button_channel_t;

static const button_port_t *s_port;
static uint32_t s_irq_sample_after_ms;
static button_transition_t s_transition_queue[BUTTON_INPUT_TRANSITION_QUEUE_DEPTH];
static uint8_t s_transition_head;
static uint8_t s_transition_tail;

static bool button_ch_power(const button_sample_t *sample)
{
    return sample->power_pressed;
}

static bool button_ch_reset(const button_sample_t *sample)
{
    return sample->reset_pressed;
}

static bool button_ch_dispense(const button_sample_t *sample)
{
    return sample->dispense_pressed;
}

static button_channel_t s_channels[] = {
    { BUTTON_ID_POWER, true, button_ch_power, false, false },
    { BUTTON_ID_RESET, false, button_ch_reset, false, false },
    { BUTTON_ID_DISPENSE, true, button_ch_dispense, false, false },
};

static void button_input_enqueue_transition(button_id_t id,
                                            button_edge_t edge,
                                            uint32_t at_ms)
{
    uint8_t next =
        (uint8_t)((s_transition_tail + 1u) % BUTTON_INPUT_TRANSITION_QUEUE_DEPTH);

    if (next == s_transition_head) {
        return;
    }

    s_transition_queue[s_transition_tail].id = id;
    s_transition_queue[s_transition_tail].edge = edge;
    s_transition_queue[s_transition_tail].at_ms = at_ms;
    s_transition_tail = next;
}

static bool button_channel_may_sample(const button_channel_t *ch, uint32_t now_ms)
{
    if (!ch->irq_backed || s_irq_sample_after_ms == 0u) {
        return true;
    }

    return now_ms >= s_irq_sample_after_ms;
}

static void button_input_update_channel(button_channel_t *ch,
                                        bool pressed,
                                        uint32_t now_ms)
{
    if (!button_channel_may_sample(ch, now_ms)) {
        return;
    }

    if (pressed == ch->last) {
        if (pressed != ch->confirmed) {
            ch->confirmed = pressed;
            button_input_enqueue_transition(
                ch->id,
                pressed ? BUTTON_EDGE_DOWN : BUTTON_EDGE_UP,
                now_ms);
        }
    }

    ch->last = pressed;
}

static void button_input_apply_sample(const button_sample_t *sample, uint32_t now_ms)
{
    size_t i;

    for (i = 0u; i < (sizeof(s_channels) / sizeof(s_channels[0])); i++) {
        button_input_update_channel(&s_channels[i],
                                    s_channels[i].is_pressed(sample),
                                    now_ms);
    }
}

static void button_input_sample(uint32_t now_ms)
{
    button_sample_t sample;
    port_err_t err;

    if (s_port == NULL || s_port->read_sample == NULL) {
        return;
    }

    err = s_port->read_sample(&sample);
    if (err != PORT_OK) {
        return;
    }

    button_input_apply_sample(&sample, now_ms);
}

void button_input_init(const button_port_t *port)
{
    s_port = port;
    button_input_reset();
}

void button_input_reset(void)
{
    size_t i;

    s_irq_sample_after_ms = 0u;
    s_transition_head = 0u;
    s_transition_tail = 0u;

    for (i = 0u; i < (sizeof(s_channels) / sizeof(s_channels[0])); i++) {
        s_channels[i].last = false;
        s_channels[i].confirmed = false;
    }
}

void button_input_notify_irq(uint32_t now_ms)
{
    uint32_t ready_at = now_ms + BUTTON_INPUT_DEBOUNCE_MS;

    if (ready_at > s_irq_sample_after_ms) {
        s_irq_sample_after_ms = ready_at;
    }
}

void button_input_poll(uint32_t now_ms)
{
    button_input_sample(now_ms);
}

bool button_input_pop_transition(button_transition_t *tr)
{
    if (tr == NULL || s_transition_head == s_transition_tail) {
        return false;
    }

    *tr = s_transition_queue[s_transition_head];
    s_transition_head =
        (uint8_t)((s_transition_head + 1u) % BUTTON_INPUT_TRANSITION_QUEUE_DEPTH);
    return true;
}
