/*
 * Retained config MQTT publisher — spec/30-processes/mqtt-protocol.md § Config snapshot
 */

#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "port_err.h"

void mqtt_config_set_device_id(const char *device_id);
void mqtt_config_publish_snapshot(void);
void mqtt_config_connect_snapshot(void);
port_err_t mqtt_config_handle(const void *payload, size_t len);
bool mqtt_config_format_snapshot(char *buf, size_t len);

void mqtt_config_test_reset(void);

#endif /* MQTT_CONFIG_H */
