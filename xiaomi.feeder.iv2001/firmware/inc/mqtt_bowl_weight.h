/*
 * Bowl weight MQTT publisher — spec/30-processes/mqtt-protocol.md § Bowl weight
 */

#ifndef MQTT_BOWL_WEIGHT_H
#define MQTT_BOWL_WEIGHT_H

#include <stdbool.h>
#include <stdint.h>

#include "bowl_grams_present.h"

#define MQTT_BOWL_WEIGHT_CHANGE_THRESHOLD_G  2
#define MQTT_BOWL_WEIGHT_COALESCE_MS         2000u

void mqtt_bowl_weight_set_device_id(const char *device_id);
void mqtt_bowl_weight_sync(bowl_grams_status_t status,
                           int32_t grams,
                           bool force);
void mqtt_bowl_weight_on_mqtt_connected(void);
void mqtt_bowl_weight_on_outbox_reset(void);

void mqtt_bowl_weight_test_reset(void);

#endif /* MQTT_BOWL_WEIGHT_H */
