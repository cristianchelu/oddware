/*
 * MQTT port — spec/40-architecture/ports.md
 */

#ifndef MQTT_PORT_H
#define MQTT_PORT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "port_err.h"

typedef struct mqtt_lwt {
    const char *topic;
    const char *payload;
    uint8_t qos;
    bool retain;
} mqtt_lwt_t;

typedef struct mqtt_connect_cfg {
    const char *host;
    uint16_t port;
    const char *client_id;
    const char *username;
    const char *password;
    mqtt_lwt_t lwt;
} mqtt_connect_cfg_t;

typedef void (*mqtt_message_cb_t)(const char *topic,
                                  const void *payload,
                                  size_t len,
                                  void *ctx);

typedef void (*mqtt_connection_cb_t)(bool connected, void *ctx);

typedef struct mqtt_port {
    port_err_t (*connect)(const mqtt_connect_cfg_t *cfg);
    port_err_t (*disconnect)(void);
    port_err_t (*publish)(const char *topic,
                          const void *payload,
                          size_t len,
                          uint8_t qos,
                          bool retain);
    port_err_t (*subscribe)(const char *topic, uint8_t qos);
    port_err_t (*set_lwt)(const mqtt_lwt_t *lwt);
    bool (*is_connected)(void);
    void (*set_callbacks)(mqtt_message_cb_t on_message,
                          mqtt_connection_cb_t on_connection,
                          void *ctx);
} mqtt_port_t;

const mqtt_port_t *mqtt_port_get(void);

#endif /* MQTT_PORT_H */
