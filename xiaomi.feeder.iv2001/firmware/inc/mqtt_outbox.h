/*
 * MQTT publish outbox — spec/30-processes/mqtt-protocol.md § Publish path
 */

#ifndef MQTT_OUTBOX_H
#define MQTT_OUTBOX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mqtt_port.h"

bool mqtt_outbox_enqueue(const char *topic,
                         const void *payload,
                         size_t len,
                         uint8_t qos,
                         bool retain);

bool mqtt_outbox_drain_one(const mqtt_port_t *mqtt);

void mqtt_outbox_reset(void);

unsigned mqtt_outbox_pending(void);

#endif /* MQTT_OUTBOX_H */
