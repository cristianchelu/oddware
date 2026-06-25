/*
 * Feed mode MQTT publisher — spec/30-processes/mqtt-protocol.md § Feed mode
 */

#ifndef MQTT_FEED_MODE_H
#define MQTT_FEED_MODE_H

#include "dispense.h"
#include "port_err.h"

void mqtt_feed_mode_set_device_id(const char *device_id);
void mqtt_feed_mode_publish_snapshot(void);
void mqtt_feed_mode_connect_snapshot(void);
port_err_t mqtt_feed_mode_apply(dispense_mode_t mode);
port_err_t mqtt_feed_mode_handle(const void *payload, size_t len);

void mqtt_feed_mode_test_reset(void);

#endif /* MQTT_FEED_MODE_H */
