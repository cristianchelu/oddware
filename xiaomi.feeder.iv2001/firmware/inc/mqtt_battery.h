/*
 * Battery MQTT publisher — spec/30-processes/mqtt-protocol.md § Battery
 */

#ifndef MQTT_BATTERY_H
#define MQTT_BATTERY_H

#include <stdbool.h>
#include <stdint.h>

#define MQTT_BATTERY_CHANGE_THRESHOLD_PCT  1u
#define MQTT_BATTERY_PAYLOAD_UNKNOWN       "unknown"

void mqtt_battery_set_device_id(const char *device_id);
void mqtt_battery_sync(bool known, uint8_t pct, bool force);
void mqtt_battery_on_mqtt_connected(void);
void mqtt_battery_on_outbox_reset(void);

void mqtt_battery_test_reset(void);

#endif /* MQTT_BATTERY_H */
