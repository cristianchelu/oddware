/*
 * MQTT dispense command — spec/30-processes/mqtt-protocol.md § HA validation slice
 */

#ifndef MQTT_DISPENSE_CMD_H
#define MQTT_DISPENSE_CMD_H

#include <stddef.h>

#define MQTT_DISPENSE_DEFAULT_PORTIONS  1u

void mqtt_dispense_cmd_handle(const char *topic,
                              const void *payload,
                              size_t len,
                              const char *device_id);

#endif /* MQTT_DISPENSE_CMD_H */
