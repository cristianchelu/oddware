/*
 * FreeRTOS app_event_q backend — spec/30-processes/app-event-loop.md
 */

#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "app_event.h"
#include "app_event_port.h"

static QueueHandle_t s_app_event_q;
static volatile uint8_t s_display_ticks_queued;
static volatile uint8_t s_timer_ticks_queued;

void app_event_port_init(void)
{
    if (s_app_event_q == NULL) {
        s_app_event_q = xQueueCreate(APP_EVENT_QUEUE_DEPTH, sizeof(app_event_t));
    }
}

bool app_event_post(const app_event_t *ev)
{
    if (ev == NULL || s_app_event_q == NULL) {
        return false;
    }

    if (ev->type == EVT_DISPLAY_TICK && s_display_ticks_queued > 0u) {
        return true;
    }

    if (ev->type == EVT_TIMER_TICK && s_timer_ticks_queued > 0u) {
        return true;
    }

    if (xQueueSend(s_app_event_q, ev, 0) != pdPASS) {
        return false;
    }

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
    TickType_t ticks = (wait_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(wait_ms);

    if (ev == NULL || s_app_event_q == NULL) {
        return false;
    }

    return xQueueReceive(s_app_event_q, ev, ticks) == pdTRUE;
}

bool app_event_try_receive(app_event_t *ev)
{
    return app_event_receive(ev, 0u);
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

    vPortFree(ev->u.mqtt_message.topic);
    vPortFree(ev->u.mqtt_message.payload);
    ev->u.mqtt_message.topic = NULL;
    ev->u.mqtt_message.payload = NULL;
    ev->u.mqtt_message.len = 0u;
}
