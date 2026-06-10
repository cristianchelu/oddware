/*
 * Provisioning STA test-connect — spec/30-processes/provisioning-flow.md
 */

#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "app_event.h"
#include "provision_wifi_try.h"

bool provision_wifi_try_connect(const char *ssid,
                                const char *pass,
                                uint32_t timeout_ms,
                                const char *ap_ssid,
                                uint8_t ap_channel,
                                const provision_wifi_try_deps_t *deps)
{
    if (ssid == NULL || pass == NULL || ap_ssid == NULL || deps == NULL) {
        return false;
    }

    if (deps->http_stop != NULL) {
        (void)deps->http_stop();
    }

    if (deps->ap_stop != NULL) {
        (void)deps->ap_stop();
    }

    {
        app_event_t ev;

        memset(&ev, 0, sizeof(ev));
        ev.type = EVT_WIFI_STA_CONNECTING;
        (void)app_event_post(&ev);
    }

    if (deps->sta_connect == NULL || deps->sta_connect(ssid, pass) != PORT_OK) {
        goto restore_ap;
    }

    if (deps->sta_wait_ready != NULL && deps->sta_wait_ready(timeout_ms)) {
        return true;
    }

restore_ap:
    if (deps->sta_abort != NULL) {
        deps->sta_abort();
    }

    vTaskDelay(pdMS_TO_TICKS(PROVISION_WIFI_TRY_AP_SETTLE_MS));

    if (deps->ap_start != NULL) {
        (void)deps->ap_start(ap_ssid, ap_channel);
    }

    {
        app_event_t ev;

        memset(&ev, 0, sizeof(ev));
        ev.type = EVT_WIFI_STA_AP_MODE;
        (void)app_event_post(&ev);
    }

    if (deps->http_start != NULL) {
        (void)deps->http_start(PROVISION_WIFI_TRY_HTTP_PORT);
    }

    return false;
}
