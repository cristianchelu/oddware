/*
 * Schedule MQTT publisher — spec/30-processes/mqtt-protocol.md § Schedule
 */

#ifndef MQTT_SCHEDULE_H
#define MQTT_SCHEDULE_H

#include <stdbool.h>
#include <stddef.h>

#include "mqtt_port.h"
#include "mqtt_route.h"

bool mqtt_schedule_handle(mqtt_route_kind_t route,
                          const void *payload,
                          size_t len);

void mqtt_schedule_request_publish(void);
void mqtt_schedule_connect_snapshot(void);
bool mqtt_schedule_drain(const mqtt_port_t *mqtt);

void mqtt_schedule_test_reset(void);

#endif /* MQTT_SCHEDULE_H */
