/*
 * Device timezone MQTT publisher — spec/30-processes/mqtt-protocol.md § Device timezone
 */

#ifndef MQTT_TIMEZONE_H
#define MQTT_TIMEZONE_H

#include <stdbool.h>

void mqtt_timezone_set_device_id(const char *device_id);
void mqtt_timezone_publish_snapshot(void);
void mqtt_timezone_connect_snapshot(void);
void mqtt_timezone_on_outbox_reset(void);
void mqtt_timezone_test_reset(void);

#endif /* MQTT_TIMEZONE_H */
