/*
 * Home Assistant MQTT discovery — spec/30-processes/mqtt-protocol.md
 */

#ifndef MQTT_HA_DISCOVERY_H
#define MQTT_HA_DISCOVERY_H

#include <stddef.h>

#define MQTT_HA_MANUFACTURER  "Xiaomi"
#define MQTT_HA_MODEL         "Smart Pet Food Feeder 2"

int mqtt_ha_format_dispense_button_config(char *buf,
                                          size_t len,
                                          const char *device_id);

int mqtt_ha_format_bowl_error_config(char *buf,
                                     size_t len,
                                     const char *device_id);

void mqtt_ha_discovery_schedule(const char *device_id);

#endif /* MQTT_HA_DISCOVERY_H */
