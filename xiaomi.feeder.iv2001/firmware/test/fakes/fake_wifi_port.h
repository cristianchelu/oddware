#ifndef FAKE_WIFI_PORT_H
#define FAKE_WIFI_PORT_H

#include <stdbool.h>

#include "wifi_port.h"

void fake_wifi_port_reset(void);
const wifi_port_t *fake_wifi_port_get(void);
void fake_wifi_port_set_sta_up(bool connected, bool has_ip);

#endif /* FAKE_WIFI_PORT_H */
