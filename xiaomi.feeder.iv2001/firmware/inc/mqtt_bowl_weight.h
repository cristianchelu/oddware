/*
 * Bowl weight MQTT publisher — spec/30-processes/mqtt-protocol.md § Bowl weight
 */

#ifndef MQTT_BOWL_WEIGHT_H
#define MQTT_BOWL_WEIGHT_H

#include <stdbool.h>
#include <stdint.h>

#include "bowl_mass_present.h"
#include "weight_units.h"

#define MQTT_BOWL_WEIGHT_CHANGE_THRESHOLD_G   2
#define MQTT_BOWL_WEIGHT_CHANGE_THRESHOLD_DG  WEIGHT_G_TO_DG(MQTT_BOWL_WEIGHT_CHANGE_THRESHOLD_G)
#define MQTT_BOWL_WEIGHT_COALESCE_MS            2000u

void mqtt_bowl_weight_set_device_id(const char *device_id);
void mqtt_bowl_weight_sync(bowl_mass_status_t status,
                           weight_dg_t dg,
                           bool force);
void mqtt_bowl_weight_on_mqtt_connected(void);
void mqtt_bowl_weight_on_outbox_reset(void);

bool mqtt_bowl_weight_format_wire(char *buf, size_t len);

void mqtt_bowl_weight_test_reset(void);

#endif /* MQTT_BOWL_WEIGHT_H */
