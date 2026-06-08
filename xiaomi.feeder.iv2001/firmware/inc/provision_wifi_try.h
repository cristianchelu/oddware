/*
 * Provisioning STA test-connect — spec/30-processes/provisioning-flow.md
 */

#ifndef PROVISION_WIFI_TRY_H
#define PROVISION_WIFI_TRY_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

#define PROVISION_WIFI_TRY_HTTP_PORT 80
#define PROVISION_WIFI_TRY_AP_SETTLE_MS 200

typedef struct provision_wifi_try_deps {
    port_err_t (*http_stop)(void);
    port_err_t (*http_start)(uint16_t port);
    port_err_t (*ap_stop)(void);
    port_err_t (*ap_start)(const char *ssid, uint8_t channel);
    port_err_t (*sta_connect)(const char *ssid, const char *pass);
    bool (*sta_wait_ready)(uint32_t timeout_ms);
    void (*sta_abort)(void);
} provision_wifi_try_deps_t;

bool provision_wifi_try_connect(const char *ssid,
                                const char *pass,
                                uint32_t timeout_ms,
                                const char *ap_ssid,
                                uint8_t ap_channel,
                                const provision_wifi_try_deps_t *deps);

#endif /* PROVISION_WIFI_TRY_H */
