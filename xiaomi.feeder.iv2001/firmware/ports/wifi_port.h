/*
 * WiFi port — spec/40-architecture/ports.md
 *
 * STA connect stages: set_credentials → radio_up → arm_connect → wait_ready
 * (wifi_session.c). connect() is set_credentials + arm_connect for provisioning.
 */

#ifndef WIFI_PORT_H
#define WIFI_PORT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

typedef struct wifi_port {
    port_err_t (*disconnect)(void);
    port_err_t (*radio_up)(void);
    port_err_t (*set_credentials)(const char *ssid, const char *pass);
    port_err_t (*arm_connect)(void);
    port_err_t (*connect)(const char *ssid, const char *pass);
    port_err_t (*wait_ready)(uint32_t timeout_ms);
    bool (*is_connected)(void);
    port_err_t (*get_ip)(char *buf, size_t len);
    port_err_t (*start_ap)(const char *ssid, const char *pass, uint8_t channel);
    port_err_t (*stop_ap)(void);
} wifi_port_t;

const wifi_port_t *wifi_port_get(void);

#endif /* WIFI_PORT_H */
