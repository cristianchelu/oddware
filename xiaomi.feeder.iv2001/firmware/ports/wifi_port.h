/*
 * WiFi port — spec/40-architecture/ports.md
 */

#ifndef WIFI_PORT_H
#define WIFI_PORT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

typedef struct wifi_port {
    port_err_t (*connect)(const char *ssid, const char *pass);
    bool (*is_connected)(void);
    port_err_t (*get_ip)(char *buf, size_t len);
    port_err_t (*start_ap)(const char *ssid, const char *pass, uint8_t channel);
    port_err_t (*stop_ap)(void);
} wifi_port_t;

const wifi_port_t *wifi_port_get(void);

#endif /* WIFI_PORT_H */
