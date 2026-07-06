/*
 * Mains MQTT publisher — spec/30-processes/mqtt-protocol.md § Mains
 */

#ifndef MQTT_MAINS_H
#define MQTT_MAINS_H

#include <stdbool.h>

void mqtt_mains_set_device_id(const char *device_id);
void mqtt_mains_sync(bool mains_connected);
void mqtt_mains_connect_snapshot(bool mains_connected);
void mqtt_mains_on_mqtt_connected(void);
void mqtt_mains_on_outbox_reset(void);

bool mqtt_mains_format_wire(char *buf, size_t len);

void mqtt_mains_test_reset(void);

#endif /* MQTT_MAINS_H */
