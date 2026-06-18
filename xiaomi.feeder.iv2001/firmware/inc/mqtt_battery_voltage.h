/*
 * Battery pack voltage MQTT publisher — spec/30-processes/mqtt-protocol.md § Battery pack voltage
 */

#ifndef MQTT_BATTERY_VOLTAGE_H
#define MQTT_BATTERY_VOLTAGE_H

#include <stdbool.h>
#include <stdint.h>

#define MQTT_BATTERY_VOLTAGE_CHANGE_THRESHOLD_MV  10u

void mqtt_battery_voltage_set_device_id(const char *device_id);
void mqtt_battery_voltage_sync(uint16_t pack_mv, bool force);
void mqtt_battery_voltage_on_mqtt_connected(void);
void mqtt_battery_voltage_on_outbox_reset(void);

void mqtt_battery_voltage_test_reset(void);

#endif /* MQTT_BATTERY_VOLTAGE_H */
