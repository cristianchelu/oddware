/*
 * Device condition MQTT publisher — spec/30-processes/mqtt-protocol.md § Device condition
 */

#ifndef MQTT_STATE_H
#define MQTT_STATE_H

#include <stdbool.h>

void mqtt_state_set_device_id(const char *device_id);
void mqtt_state_sync(bool bowl_error);
void mqtt_state_on_mqtt_connected(void);

void mqtt_state_on_outbox_reset(void);

bool mqtt_state_format_bowl_error(bool *out_bowl_error);

void mqtt_state_test_reset(void);

#endif /* MQTT_STATE_H */
