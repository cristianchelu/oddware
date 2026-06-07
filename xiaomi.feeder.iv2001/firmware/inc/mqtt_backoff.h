/*
 * MQTT reconnect backoff — spec/30-processes/mqtt-protocol.md
 */

#ifndef MQTT_BACKOFF_H
#define MQTT_BACKOFF_H

#include <stdint.h>

#define MQTT_BACKOFF_INITIAL_MS 1000u
#define MQTT_BACKOFF_MAX_MS     60000u

typedef struct mqtt_backoff {
    uint32_t delay_ms;
} mqtt_backoff_t;

void mqtt_backoff_init(mqtt_backoff_t *backoff);
void mqtt_backoff_on_failure(mqtt_backoff_t *backoff);
void mqtt_backoff_on_success(mqtt_backoff_t *backoff);
uint32_t mqtt_backoff_current_ms(const mqtt_backoff_t *backoff);

#endif /* MQTT_BACKOFF_H */
