/*
 * Home Assistant MQTT discovery — spec/30-processes/mqtt-protocol.md
 */

#ifndef MQTT_HA_DISCOVERY_H
#define MQTT_HA_DISCOVERY_H

#include <stddef.h>

#define MQTT_HA_MANUFACTURER  "Oddware"
#define MQTT_HA_MODEL         "IV2001 Pet Feeder"

int mqtt_ha_format_dispense_button_config(char *buf,
                                          size_t len,
                                          const char *device_id);

void mqtt_ha_discovery_schedule(const char *device_id);

#endif /* MQTT_HA_DISCOVERY_H */
