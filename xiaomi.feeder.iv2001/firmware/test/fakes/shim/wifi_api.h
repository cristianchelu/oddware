#ifndef WIFI_API_H
#define WIFI_API_H

#include <stdint.h>

typedef uint8_t wifi_hal_port_t;

#define WIFI_PORT_STA 0

int wifi_config_get_mac_address(wifi_hal_port_t port, uint8_t mac[6]);

#endif /* WIFI_API_H */
