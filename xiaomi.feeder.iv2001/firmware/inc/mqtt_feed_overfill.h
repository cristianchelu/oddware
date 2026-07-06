/*
 * Overfill protection MQTT publisher — spec/30-processes/mqtt-protocol.md § Overfill protection
 */

#ifndef MQTT_FEED_OVERFILL_H
#define MQTT_FEED_OVERFILL_H

#include <stddef.h>
#include <stdbool.h>

#include "port_err.h"

bool mqtt_feed_overfill_format_snapshot(char *buf, size_t len);
void mqtt_feed_overfill_set_device_id(const char *device_id);
void mqtt_feed_overfill_publish_snapshot(void);
void mqtt_feed_overfill_connect_snapshot(void);
port_err_t mqtt_feed_overfill_handle(const void *payload, size_t len);

void mqtt_feed_overfill_test_reset(void);

#endif /* MQTT_FEED_OVERFILL_H */
