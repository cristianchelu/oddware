/*
 * AP provisioning orchestration — spec/30-processes/provisioning-flow.md
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "syslog.h"

#include "hal_cache.h"
#include "hal_sys.h"
#include "wifi_api.h"
#include "wifi_lwip_helper.h"

#include "config_port.h"
#include "http_server_adapter.h"
#include "mqtt_adapter.h"
#include "mqtt_client.h"
#include "mqtt_cred.h"
#include "provision_flow.h"
#include "provision_form.h"
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

static bool provision_wifi_try_connect(const char *ssid, const char *pass, uint32_t timeout_ms)
{
    const wifi_port_t *wifi = wifi_port_get();
    const http_server_port_t *http = http_server_port_get();

    if (http->stop() != PORT_OK) {
        LOG_E(provision, "failed to stop HTTP server for STA test");
    }

    if (wifi->stop_ap() != PORT_OK) {
        LOG_E(provision, "failed to leave AP mode");
        return false;
    }

    if (wifi->connect(ssid, pass) != PORT_OK) {
        LOG_E(provision, "STA connect API failed");
        goto restore_ap;
    }

    lwip_net_ready();

    if (provision_wait_wifi_ready(timeout_ms)) {
        return true;
    }

    LOG_E(provision, "STA connect timed out");

restore_ap:
    if (wifi->start_ap(s_ap_ssid, "", PROVISION_AP_CHANNEL) != PORT_OK) {
        LOG_E(provision, "failed to restore AP mode");
        return false;
    }

    if (http->start(80) != PORT_OK) {
        LOG_E(provision, "failed to restart HTTP server");
    }

    return false;
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
    .wifi_try_connect = provision_wifi_try_connect,
    .mqtt_try_connect = provision_mqtt_try_connect,
};

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
    provision_build_ap_ssid(s_ap_ssid, sizeof(s_ap_ssid));

    if (wifi->start_ap(s_ap_ssid, "", PROVISION_AP_CHANNEL) != PORT_OK) {
        LOG_E(provision, "failed to start AP \"%s\"", s_ap_ssid);
        vTaskDelete(NULL);
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(PROVISION_AP_SETTLE_MS));

    if (http->start(80) != PORT_OK) {
        LOG_E(provision, "failed to start HTTP server");
        vTaskDelete(NULL);
        return;
    }

    s_active = true;
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

    hal_cache_disable();
    hal_cache_deinit();
    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
    return true;
}

size_t provision_handle_get(char *html, size_t len)
{
    if (html == NULL || len == 0) {
        return 0;
    }

    return provision_form_render(NULL, NULL, html, len);
}

size_t provision_handle_post(const char *body, size_t body_len, char *html, size_t len)
{
    provision_input_t input;
    provision_flow_result_t result;
    size_t html_len;

    if (html == NULL || len == 0) {
        return 0;
    }

    if (body == NULL || body_len == 0) {
        return provision_form_render(NULL, PROVISION_MSG_VALIDATION, html, len);
    }

    if (provision_form_parse_urlencoded(body, body_len, &input) != PORT_OK) {
        return provision_form_render(NULL, PROVISION_MSG_VALIDATION, html, len);
    }

    result = provision_flow_submit(&input, config_port_get(), &s_flow_ops);

    if (result == PROVISION_FLOW_OK) {
        html_len = provision_form_render_success(html, len);
        if (html_len > 0) {
            provision_reboot();
        }
        return html_len;
    }

    return provision_form_render(&input, provision_flow_message(result), html, len);
}
