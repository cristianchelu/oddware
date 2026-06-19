/*
 * Hopper level MQTT publisher — spec/30-processes/mqtt-protocol.md § hopper
 */

#ifndef MQTT_HOPPER_H
#define MQTT_HOPPER_H

#include <stdbool.h>

#include "hopper_level.h"

void mqtt_hopper_set_device_id(const char *device_id);
void mqtt_hopper_sync(hopper_level_state_t level);
void mqtt_hopper_connect_snapshot(hopper_level_state_t level);
void mqtt_hopper_on_mqtt_connected(void);
void mqtt_hopper_on_outbox_reset(void);
void mqtt_hopper_test_reset(void);

#endif /* MQTT_HOPPER_H */
