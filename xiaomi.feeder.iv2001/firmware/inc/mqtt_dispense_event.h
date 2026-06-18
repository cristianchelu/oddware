/*
 * Dispense completion MQTT event — spec/30-processes/mqtt-protocol.md § Dispense event
 */

#ifndef MQTT_DISPENSE_EVENT_H
#define MQTT_DISPENSE_EVENT_H

#include <stdbool.h>
#include <stdint.h>

#include "dispense.h"

void mqtt_dispense_event_set_device_id(const char *device_id);
bool mqtt_dispense_event_publish(const dispense_completion_t *completion);

void mqtt_dispense_event_test_reset(void);

#endif /* MQTT_DISPENSE_EVENT_H */
