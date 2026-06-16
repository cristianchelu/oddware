/*
 * Wi-Fi session orchestration — spec/30-processes/wifi-lifecycle.md
 */

#ifndef WIFI_SESSION_H
#define WIFI_SESSION_H

#include <stdint.h>

#include "port_err.h"

/*
 * Bank-B boots need headroom for N9 seek_and_connect before DHCP
 * (spec/30-processes/wifi-lifecycle.md § Bank-B boot delay).
 * 30 s wait_ready caused connect to abort before DHCP, clearing the Wi-Fi icon.
 */
#define WIFI_SESSION_CONNECT_TIMEOUT_MS 60000u

port_err_t wifi_session_down(void);
port_err_t wifi_session_up(void);
port_err_t wifi_session_connect(const char *ssid, const char *pass, uint32_t timeout_ms);

#endif /* WIFI_SESSION_H */
