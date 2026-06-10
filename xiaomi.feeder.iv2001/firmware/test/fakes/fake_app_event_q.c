/*
 * Host FIFO app_event_q — spec/30-processes/app-event-loop.md
 */

#include <stdlib.h>
#include <string.h>

#include "app_event.h"
#include "app_event_port.h"

static app_event_t s_queue[APP_EVENT_QUEUE_DEPTH];
static size_t s_head;
static size_t s_tail;
static size_t s_count;
static uint8_t s_display_ticks_queued;
static uint8_t s_timer_ticks_queued;

void app_event_port_init(void)
{
    s_head = 0u;
    s_tail = 0u;
    s_count = 0u;
    s_display_ticks_queued = 0u;
    s_timer_ticks_queued = 0u;
}

bool app_event_post(const app_event_t *ev)
{
    if (ev == NULL || s_count >= APP_EVENT_QUEUE_DEPTH) {
        return false;
    }

    if (ev->type == EVT_DISPLAY_TICK && s_display_ticks_queued > 0u) {
        return true;
    }

    if (ev->type == EVT_TIMER_TICK && s_timer_ticks_queued > 0u) {
        return true;
    }

    s_queue[s_tail] = *ev;
    s_tail = (s_tail + 1u) % APP_EVENT_QUEUE_DEPTH;
    s_count++;

    if (ev->type == EVT_DISPLAY_TICK) {
        s_display_ticks_queued++;
    }
    if (ev->type == EVT_TIMER_TICK) {
        s_timer_ticks_queued++;
    }

    return true;
}

bool app_event_receive(app_event_t *ev, uint32_t wait_ms)
{
    (void)wait_ms;
    return app_event_try_receive(ev);
}

bool app_event_try_receive(app_event_t *ev)
{
    if (ev == NULL || s_count == 0u) {
        return false;
    }

    *ev = s_queue[s_head];
    s_head = (s_head + 1u) % APP_EVENT_QUEUE_DEPTH;
    s_count--;
    return true;
}

void app_event_release(app_event_t *ev)
{
    if (ev == NULL) {
        return;
    }

    if (ev->type == EVT_DISPLAY_TICK && s_display_ticks_queued > 0u) {
        s_display_ticks_queued--;
    }
    if (ev->type == EVT_TIMER_TICK && s_timer_ticks_queued > 0u) {
        s_timer_ticks_queued--;
    }

    if (ev->type != EVT_MQTT_MESSAGE) {
        return;
    }

    free(ev->u.mqtt_message.topic);
    free(ev->u.mqtt_message.payload);
    ev->u.mqtt_message.topic = NULL;
    ev->u.mqtt_message.payload = NULL;
    ev->u.mqtt_message.len = 0u;
}

void fake_app_event_q_reset(void)
{
    app_event_port_init();
}

size_t fake_app_event_q_depth(void)
{
    return s_count;
}
