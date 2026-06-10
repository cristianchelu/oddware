/*
 * AP provisioning orchestration — spec/30-processes/provisioning-flow.md
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "syslog.h"

#include "hal_cache.h"
#include "hal_sys.h"
#include "wifi_api.h"

#include "config_port.h"
#include "http_server_adapter.h"
#include "wifi_adapter.h"
#include "mqtt_adapter.h"
#include "mqtt_client.h"
#include "mqtt_cred.h"
#include "provision_flow.h"
#include "provision_form.h"
#include "provision_portal.h"
#include "provision_wifi_try.h"
#include "app_event.h"
#include "provision.h"
#include "provision_reset.h"
#include "task_def.h"
#include "wifi_cred.h"
#include "wifi_port.h"

log_create_module(provision, PRINT_LEVEL_INFO);

#define PROVISION_AP_CHANNEL        6
#define PROVISION_AP_SETTLE_MS      2000
#define PROVISION_SCHEDULER_DELAY_MS 200

static bool s_active;
static char s_ap_ssid[20];
static provision_scan_list_t s_scan_list;

#define PROVISION_SCAN_TIMEOUT_MS   10000
#define PROVISION_RESTORE_TIMER_MS  250

static void provision_refresh_scan(void)
{
    provision_scan_ap_t raw[PROVISION_SCAN_MAX_APS];
    size_t raw_count;

    provision_scan_list_clear(&s_scan_list);
    raw_count = wifi_adapter_scan_networks(raw, PROVISION_SCAN_MAX_APS, PROVISION_SCAN_TIMEOUT_MS);
    provision_scan_list_merge(&s_scan_list, raw, raw_count, s_ap_ssid);
}

static void provision_get_mac_hex(char *buf, size_t len)
{
    uint8_t mac[6];
    int written;
    int i;

    if (buf == NULL || len < 13) {
        if (buf != NULL && len > 0) {
            buf[0] = '\0';
        }
        return;
    }

    if (wifi_config_get_mac_address(WIFI_PORT_STA, mac) < 0) {
        buf[0] = '\0';
        return;
    }

    written = 0;
    for (i = 0; i < 6; i++) {
        written += snprintf(buf + written, len - (size_t)written, "%02x", mac[i]);
    }
}

static void provision_build_ap_ssid(char *buf, size_t len)
{
    char mac_hex[13];

    if (buf == NULL || len < 13) {
        return;
    }

    provision_get_mac_hex(mac_hex, sizeof(mac_hex));
    if (strlen(mac_hex) < 4) {
        snprintf(buf, len, "PetFeeder-0000");
        return;
    }

    snprintf(buf, len, "PetFeeder-%s", mac_hex + strlen(mac_hex) - 4);
}

static bool provision_wait_wifi_ready(uint32_t timeout_ms)
{
    const wifi_port_t *wifi = wifi_port_get();
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    char ip[20];

    while (xTaskGetTickCount() < deadline) {
        if (wifi->is_connected() && wifi->get_ip(ip, sizeof(ip)) == PORT_OK) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    return false;
}

static port_err_t provision_http_stop(void)
{
    const http_server_port_t *http = http_server_port_get();

    if (http->stop() != PORT_OK) {
        LOG_E(provision, "failed to stop HTTP server");
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static port_err_t provision_http_start(uint16_t port)
{
    const http_server_port_t *http = http_server_port_get();

    if (http->start(port) != PORT_OK) {
        LOG_E(provision, "failed to restart HTTP server");
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static port_err_t provision_ap_stop(void)
{
    const wifi_port_t *wifi = wifi_port_get();

    if (wifi->stop_ap() != PORT_OK) {
        LOG_E(provision, "failed to leave AP mode");
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static port_err_t provision_ap_start(const char *ssid, uint8_t channel)
{
    const wifi_port_t *wifi = wifi_port_get();

    if (wifi->start_ap(ssid, "", channel) != PORT_OK) {
        LOG_E(provision, "failed to restore AP mode");
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static port_err_t provision_sta_connect(const char *ssid, const char *pass)
{
    const wifi_port_t *wifi = wifi_port_get();

    if (wifi->connect(ssid, pass) != PORT_OK) {
        LOG_E(provision, "STA connect API failed");
        return PORT_ERR_IO;
    }

    /* lwip_net_ready() blocks until DHCP; provision_wait_wifi_ready times out instead. */
    return PORT_OK;
}

static void provision_sta_abort(void)
{
    wifi_adapter_clear_sdk_sta_profile();
}

static const provision_wifi_try_deps_t s_wifi_try_deps = {
    .http_stop = NULL,
    .http_start = NULL,
    .ap_stop = provision_ap_stop,
    .ap_start = provision_ap_start,
    .sta_connect = provision_sta_connect,
    .sta_wait_ready = provision_wait_wifi_ready,
    .sta_abort = provision_sta_abort,
};

static provision_portal_restore_t s_pending_restore;
static provision_portal_restore_t s_timer_restore_kind;
static TimerHandle_t s_restore_timer;

static void provision_restore_ap_portal(void);

static void provision_restore_http_only(void)
{
    if (http_server_adapter_force_restart(PROVISION_WIFI_TRY_HTTP_PORT) != PORT_OK) {
        LOG_E(provision, "failed to force-restart HTTP server");
    }
}

static void provision_restore_timer_cb(TimerHandle_t timer)
{
    (void)timer;

    switch (s_timer_restore_kind) {
    case PROVISION_PORTAL_RESTORE_HTTP_ONLY:
        provision_restore_http_only();
        break;
    case PROVISION_PORTAL_RESTORE_AP_PORTAL:
        provision_restore_ap_portal();
        break;
    default:
        break;
    }

    s_timer_restore_kind = PROVISION_PORTAL_RESTORE_NONE;
}

static bool provision_ensure_restore_timer(void)
{
    if (s_restore_timer != NULL) {
        return true;
    }

    s_restore_timer = xTimerCreate("prov_rst",
                                   pdMS_TO_TICKS(PROVISION_RESTORE_TIMER_MS),
                                   pdFALSE,
                                   NULL,
                                   provision_restore_timer_cb);
    return s_restore_timer != NULL;
}

static void provision_schedule_restore(provision_portal_restore_t kind)
{
    if (kind == PROVISION_PORTAL_RESTORE_NONE) {
        return;
    }

    if (s_pending_restore == PROVISION_PORTAL_RESTORE_NONE) {
        s_pending_restore = kind;
    } else if (kind == PROVISION_PORTAL_RESTORE_AP_PORTAL) {
        s_pending_restore = PROVISION_PORTAL_RESTORE_AP_PORTAL;
    }
}

static void provision_portal_request_restore(provision_portal_restore_t kind)
{
    provision_schedule_restore(kind);
}

static void provision_restore_ap_portal(void)
{
    provision_sta_abort();
    vTaskDelay(pdMS_TO_TICKS(PROVISION_WIFI_TRY_AP_SETTLE_MS));
    (void)provision_ap_start(s_ap_ssid, PROVISION_AP_CHANNEL);
    provision_restore_http_only();
}

static bool provision_flow_wifi_try_connect(const char *ssid, const char *pass, uint32_t timeout_ms)
{
    return provision_wifi_try_connect(ssid,
                                      pass,
                                      timeout_ms,
                                      s_ap_ssid,
                                      PROVISION_AP_CHANNEL,
                                      &s_wifi_try_deps);
}

static bool provision_mqtt_try_connect(const provision_input_t *input, uint32_t timeout_ms)
{
    char mac_hex[13];

    if (input == NULL) {
        return false;
    }

    provision_get_mac_hex(mac_hex, sizeof(mac_hex));
    return mqtt_adapter_probe_broker(input, mac_hex, timeout_ms);
}

static void provision_reboot(void)
{
    vTaskDelay(pdMS_TO_TICKS(3000));
    hal_cache_disable();
    hal_cache_deinit();
    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
}

static const provision_flow_ops_t s_flow_ops = {
    .save_wifi = NULL,
    .save_mqtt = NULL,
    .wifi_try_connect = provision_flow_wifi_try_connect,
    .mqtt_try_connect = provision_mqtt_try_connect,
};

static provision_flow_result_t provision_portal_flow_submit(const provision_input_t *input)
{
    return provision_flow_submit(input, config_port_get(), &s_flow_ops);
}

static const provision_portal_deps_t s_portal_deps = {
    .scan = &s_scan_list,
    .active = false,
    .refresh_scan = provision_refresh_scan,
    .flow_submit = provision_portal_flow_submit,
    .request_restore = provision_portal_request_restore,
    .on_success = provision_reboot,
};

void provision_after_cgi_response(void)
{
    provision_portal_restore_t kind = s_pending_restore;

    if (kind == PROVISION_PORTAL_RESTORE_NONE) {
        return;
    }

    if (!provision_ensure_restore_timer()) {
        LOG_E(provision, "failed to create portal restore timer");
        return;
    }

    s_pending_restore = PROVISION_PORTAL_RESTORE_NONE;
    s_timer_restore_kind = kind;

    if (xTimerChangePeriod(s_restore_timer,
                           pdMS_TO_TICKS(PROVISION_RESTORE_TIMER_MS),
                           pdMS_TO_TICKS(100)) != pdPASS) {
        LOG_E(provision, "failed to arm portal restore timer");
        s_pending_restore = kind;
        return;
    }

    if (xTimerStart(s_restore_timer, pdMS_TO_TICKS(100)) != pdPASS) {
        LOG_E(provision, "failed to start portal restore timer");
        s_pending_restore = kind;
    }
}

bool provision_is_active(void)
{
    return s_active;
}

static void provision_task(void *param)
{
    const wifi_port_t *wifi = wifi_port_get();
    const http_server_port_t *http = http_server_port_get();

    (void)param;

    vTaskDelay(pdMS_TO_TICKS(PROVISION_SCHEDULER_DELAY_MS));

    mqtt_client_stop();
    wifi_adapter_clear_sdk_sta_profile();
    provision_build_ap_ssid(s_ap_ssid, sizeof(s_ap_ssid));

    if (wifi->start_ap(s_ap_ssid, "", PROVISION_AP_CHANNEL) != PORT_OK) {
        LOG_E(provision, "failed to start AP \"%s\"", s_ap_ssid);
        vTaskDelete(NULL);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(PROVISION_AP_SETTLE_MS));
    provision_refresh_scan();

    if (http->start(80) != PORT_OK) {
        LOG_E(provision, "failed to start HTTP server");
        vTaskDelete(NULL);
        return;
    }

    s_active = true;
    {
        app_event_t ev;

        memset(&ev, 0, sizeof(ev));
        ev.type = EVT_WIFI_STA_AP_MODE;
        (void)app_event_post(&ev);
    }
    LOG_I(provision, "AP provisioning active — SSID %s", s_ap_ssid);
    printf("[provision] AP %s — open http://192.168.4.1/\r\n", s_ap_ssid);

    vTaskDelete(NULL);
}

void provision_start(void)
{
    if (wifi_cred_is_stored(config_port_get())) {
        return;
    }

    if (xTaskCreate(provision_task,
                    "provision",
                    APP_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                    NULL,
                    APP_TASK_PRIO,
                    NULL) != pdPASS) {
        LOG_E(provision, "failed to start provision task");
    }
}

bool provision_factory_reset(void)
{
    if (!provision_erase_app_groups(config_port_get())) {
        return false;
    }

    wifi_adapter_clear_sdk_sta_profile();

    hal_cache_disable();
    hal_cache_deinit();
    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
    return true;
}

size_t provision_handle_get(const char *query, size_t query_len, char *html, size_t len)
{
    provision_portal_deps_t deps = s_portal_deps;

    deps.active = s_active;
    return provision_portal_handle_get(query, query_len, html, len, &deps);
}

size_t provision_handle_post(const char *body, size_t body_len, char *html, size_t len)
{
    provision_portal_deps_t deps = s_portal_deps;

    deps.active = s_active;
    return provision_portal_handle_post(body, body_len, html, len, &deps);
}
