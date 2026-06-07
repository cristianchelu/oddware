/*
 * MQTT reconnect backoff — spec/30-processes/mqtt-protocol.md
 */

#include <stddef.h>

#include "mqtt_backoff.h"

void mqtt_backoff_init(mqtt_backoff_t *backoff)
{
    if (backoff != NULL) {
        backoff->delay_ms = MQTT_BACKOFF_INITIAL_MS;
    }
}

void mqtt_backoff_on_failure(mqtt_backoff_t *backoff)
{
    uint32_t next;

    if (backoff == NULL) {
        return;
    }

    next = backoff->delay_ms * 2u;
    if (next > MQTT_BACKOFF_MAX_MS) {
        next = MQTT_BACKOFF_MAX_MS;
    }
    backoff->delay_ms = next;
}

void mqtt_backoff_on_success(mqtt_backoff_t *backoff)
{
    mqtt_backoff_init(backoff);
}

uint32_t mqtt_backoff_current_ms(const mqtt_backoff_t *backoff)
{
    if (backoff == NULL) {
        return MQTT_BACKOFF_INITIAL_MS;
    }

    return backoff->delay_ms;
}
