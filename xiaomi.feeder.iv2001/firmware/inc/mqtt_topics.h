/*
 * MQTT topic helpers — spec/30-processes/mqtt-protocol.md
 */

#ifndef MQTT_TOPICS_H
#define MQTT_TOPICS_H

#include <stddef.h>

#include "port_err.h"

#define MQTT_TOPIC_PREFIX "petfeeder"

port_err_t mqtt_topic_format(char *buf,
                             size_t len,
                             const char *device_id,
                             const char *suffix);

port_err_t mqtt_client_id_format(char *buf, size_t len, const char *device_id);

port_err_t mqtt_device_id_from_mac(char *buf,
                                   size_t len,
                                   const char *mac_hex12);

#endif /* MQTT_TOPICS_H */
