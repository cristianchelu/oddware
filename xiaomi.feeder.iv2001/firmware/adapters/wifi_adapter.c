/*
 * WiFi port adapter — spec/40-architecture/ports.md
 */

#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "nvdm.h"
#include "wifi_api.h"
#include "wifi_lwip_helper.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "ethernetif.h"

#include "app_log.h"
#include "wifi_adapter.h"
#include "wifi_port.h"
#include "wifi_private_api.h"
#include "wifi_sdk_profile.h"

void wifi_adapter_clear_sdk_sta_profile(void)
{
    wifi_sdk_profile_invalidate();
}

static void wifi_adapter_set_ap_network_profile(void)
{
    nvdm_write_data_item("network",
                         "IpAddr",
                         NVDM_DATA_ITEM_TYPE_STRING,
                         (const uint8_t *)"192.168.4.1",
                         (uint32_t)strlen("192.168.4.1"));
    nvdm_write_data_item("network",
                         "IpNetmask",
                         NVDM_DATA_ITEM_TYPE_STRING,
                         (const uint8_t *)"255.255.255.0",
                         (uint32_t)strlen("255.255.255.0"));
    nvdm_write_data_item("network",
                         "IpGateway",
                         NVDM_DATA_ITEM_TYPE_STRING,
                         (const uint8_t *)"192.168.4.1",
                         (uint32_t)strlen("192.168.4.1"));
}

/*
 * Wipe SDK-internal STA caches that poison association after reboot.
 *
 * wifi_init() reads ALL STA/* NVDM keys and loads them into the N9
 * coprocessor.  After a warm reboot (OTA) the N9 RAM may still hold a
 * stale PMKSA from the previous session.  The AP has already evicted its
 * side, so the cached PMK causes MIC failure on msg 3 and a ~38 s gap
 * between scan-match and connect-start in the N9 ROM.
 *
 * Wiping the STA/* profile forces wifi_init() to hand the N9 a blank
 * slate.  Credentials are re-set via set_ssid/set_psk before
 * reload_setting() in wifi_port_connect().
 *
 * Must run BEFORE wifi_init() which reads these keys.
 */
static void wifi_adapter_wipe_sta_caches(void)
{
    const char zero[] = "0";
    uint8_t zeros[32 + 64 + 32];

    memset(zeros, 0, sizeof(zeros));

    /* Disable fast-PMK lookup in N9 ROM. */
    nvdm_write_data_item("common",
                         "StaFastLink",
                         NVDM_DATA_ITEM_TYPE_STRING,
                         (const uint8_t *)zero,
                         1);

    /* Zero the cached SSID+PSK+PMK tuple. */
    nvdm_write_data_item("STA",
                         "PMK_INFO",
                         NVDM_DATA_ITEM_TYPE_STRING,
                         zeros,
                         sizeof(zeros));

    /* Blank the STA profile so wifi_init() does not pre-load stale
     * credentials into the N9.  We re-set them in wifi_port_connect(). */
    nvdm_write_data_item("STA", "SsidLen",
                         NVDM_DATA_ITEM_TYPE_STRING,
                         (const uint8_t *)zero, 1);
    nvdm_write_data_item("STA", "Ssid",
                         NVDM_DATA_ITEM_TYPE_STRING,
                         (const uint8_t *)"", 0);
    nvdm_write_data_item("STA", "WpaPskLen",
                         NVDM_DATA_ITEM_TYPE_STRING,
                         (const uint8_t *)zero, 1);
    nvdm_write_data_item("STA", "WpaPsk",
                         NVDM_DATA_ITEM_TYPE_STRING,
                         (const uint8_t *)"", 0);

    APP_LOG_I("wifi", "wiped STA profile + PMK caches");
}

static bool s_wifi_init_complete;

static int32_t wifi_adapter_init_complete_handler(wifi_event_t event,
                                                  uint8_t *payload,
                                                  uint32_t length)
{
    (void)payload;
    (void)length;

    if (event == WIFI_EVENT_IOT_INIT_COMPLETE) {
        s_wifi_init_complete = true;
        APP_LOG_I("wifi", "IOT_INIT_COMPLETE");
    }

    return 0;
}

void wifi_adapter_stack_init(void)
{
    wifi_config_t config;
    wifi_config_ext_t ext;
    TickType_t t0;

    memset(&config, 0, sizeof(config));
    memset(&ext, 0, sizeof(ext));

    config.opmode = WIFI_MODE_STA_ONLY;
    ext.sta_auto_connect_present = 1;
    ext.sta_auto_connect = 0;

    wifi_adapter_wipe_sta_caches();

    if (wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE,
                                               wifi_adapter_init_complete_handler) < 0) {
        APP_LOG_W("wifi", "INIT_COMPLETE handler register failed");
    }

    t0 = xTaskGetTickCount();
    wifi_init(&config, &ext);
    APP_LOG_I("wifi", "wifi_init done +%lu ms",
              (unsigned long)(xTaskGetTickCount() - t0) * portTICK_PERIOD_MS);

    lwip_network_init(config.opmode);
    lwip_net_start(config.opmode);
    APP_LOG_I("wifi", "stack_init complete +%lu ms",
              (unsigned long)(xTaskGetTickCount() - t0) * portTICK_PERIOD_MS);

    if (!s_wifi_init_complete) {
        s_wifi_init_complete = true;
    }
}

static port_err_t wifi_port_connect(const char *ssid, const char *pass)
{
    uint8_t ssid_len;
    uint8_t pass_len;

    if (ssid == NULL || pass == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    ssid_len = (uint8_t)strlen(ssid);
    pass_len = (uint8_t)strlen(pass);

    if (ssid_len == 0 || ssid_len > WIFI_MAX_LENGTH_OF_SSID) {
        return PORT_ERR_INVALID_ARG;
    }

    if (pass_len > WIFI_LENGTH_PASSPHRASE) {
        return PORT_ERR_INVALID_ARG;
    }

    APP_LOG_I("wifi", "connect: ssid=\"%s\" tick=%lu",
              ssid, (unsigned long)xTaskGetTickCount());

    if (wifi_config_set_ssid(WIFI_PORT_STA, (uint8_t *)ssid, ssid_len) < 0) {
        return PORT_ERR_IO;
    }

    if (pass_len > 0) {
        if (wifi_config_set_wpa_psk_key(WIFI_PORT_STA, (uint8_t *)pass, pass_len) < 0) {
            return PORT_ERR_IO;
        }
    } else if (wifi_config_set_security_mode(WIFI_PORT_STA,
                                               WIFI_AUTH_MODE_OPEN,
                                               WIFI_ENCRYPT_TYPE_WEP_DISABLED) < 0) {
        return PORT_ERR_IO;
    }

    APP_LOG_I("wifi", "reload_setting tick=%lu",
              (unsigned long)xTaskGetTickCount());

    if (wifi_config_reload_setting() < 0) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static port_err_t wifi_port_disconnect(void)
{
    lwip_net_stop(WIFI_MODE_STA_ONLY);
    (void)wifi_connection_disconnect_ap();

    if (wifi_config_set_radio(0) < 0) {
        APP_LOG_W("wifi", "set_radio(0) failed");
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static port_err_t wifi_port_radio_up(void)
{
    if (wifi_config_set_radio(1) < 0) {
        APP_LOG_W("wifi", "set_radio(1) failed");
        return PORT_ERR_IO;
    }

    lwip_net_start(WIFI_MODE_STA_ONLY);
    return PORT_OK;
}

static bool wifi_port_is_connected(void);
static port_err_t wifi_port_get_ip(char *buf, size_t len);

static port_err_t wifi_port_wait_ready(uint32_t timeout_ms)
{
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    TickType_t init_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(5000);
    char ip[20];

    while (!s_wifi_init_complete && xTaskGetTickCount() < init_deadline) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (!s_wifi_init_complete) {
        APP_LOG_W("wifi", "wait_ready: stack init not complete");
        return PORT_ERR_IO;
    }

    while (xTaskGetTickCount() < deadline) {
        if (wifi_port_is_connected() && wifi_port_get_ip(ip, sizeof(ip)) == PORT_OK) {
            return PORT_OK;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return PORT_ERR_IO;
}

static bool wifi_port_is_connected(void)
{
    uint8_t link_status = 0;

    if (wifi_connection_get_link_status(&link_status) < 0) {
        return false;
    }

    return link_status != 0;
}

static port_err_t wifi_port_get_ip(char *buf, size_t len)
{
    struct netif *sta_if;

    if (buf == NULL || len == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    sta_if = netif_find_by_type(NETIF_TYPE_STA);
    if (sta_if == NULL || ip4_addr_isany_val(sta_if->ip_addr)) {
        return PORT_ERR_NOT_FOUND;
    }

    {
        const char *ip = inet_ntoa(sta_if->ip_addr);

        if (ip == NULL) {
            return PORT_ERR_IO;
        }

        if (strlen(ip) + 1 > len) {
            return PORT_ERR_INVALID_ARG;
        }

        strcpy(buf, ip);
    }

    return PORT_OK;
}

static port_err_t wifi_port_start_ap(const char *ssid, const char *pass, uint8_t channel)
{
    uint8_t ssid_len;
    uint8_t pass_len;
    uint8_t ap_channel = (channel == 0) ? 6 : channel;

    if (ssid == NULL || pass == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    ssid_len = (uint8_t)strlen(ssid);
    pass_len = (uint8_t)strlen(pass);

    if (ssid_len == 0 || ssid_len > WIFI_MAX_LENGTH_OF_SSID) {
        return PORT_ERR_INVALID_ARG;
    }

    if (pass_len > WIFI_LENGTH_PASSPHRASE) {
        return PORT_ERR_INVALID_ARG;
    }

    wifi_adapter_set_ap_network_profile();

    if (wifi_config_set_ssid(WIFI_PORT_AP, (uint8_t *)ssid, ssid_len) < 0) {
        return PORT_ERR_IO;
    }

    if (pass_len > 0) {
        if (wifi_config_set_wpa_psk_key(WIFI_PORT_AP, (uint8_t *)pass, pass_len) < 0) {
            return PORT_ERR_IO;
        }
        if (wifi_config_set_security_mode(WIFI_PORT_AP,
                                          WIFI_AUTH_MODE_WPA2_PSK,
                                          WIFI_ENCRYPT_TYPE_AES_ENABLED) < 0) {
            return PORT_ERR_IO;
        }
    } else if (wifi_config_set_security_mode(WIFI_PORT_AP,
                                               WIFI_AUTH_MODE_OPEN,
                                               WIFI_ENCRYPT_TYPE_WEP_DISABLED) < 0) {
        return PORT_ERR_IO;
    }

    if (wifi_config_reload_setting() < 0) {
        return PORT_ERR_IO;
    }

    if (wifi_set_opmode(WIFI_MODE_AP_ONLY) != 0) {
        return PORT_ERR_IO;
    }

    if (wifi_config_set_channel(WIFI_PORT_AP, ap_channel) < 0) {
        return PORT_ERR_IO;
    }

    if (wifi_config_reload_setting() < 0) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static port_err_t wifi_port_stop_ap(void)
{
    if (wifi_set_opmode(WIFI_MODE_STA_ONLY) != 0) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static const wifi_port_t s_wifi_port = {
    .disconnect = wifi_port_disconnect,
    .radio_up = wifi_port_radio_up,
    .connect = wifi_port_connect,
    .wait_ready = wifi_port_wait_ready,
    .is_connected = wifi_port_is_connected,
    .get_ip = wifi_port_get_ip,
    .start_ap = wifi_port_start_ap,
    .stop_ap = wifi_port_stop_ap,
};

const wifi_port_t *wifi_port_get(void)
{
    return &s_wifi_port;
}

#define WIFI_ADAPTER_SCAN_TABLE_SIZE 16

static wifi_scan_list_item_t s_scan_table[WIFI_ADAPTER_SCAN_TABLE_SIZE];
static SemaphoreHandle_t s_scan_done_sem;
static bool s_scan_handler_registered;

static int32_t wifi_adapter_scan_event_handler(wifi_event_t event,
                                               uint8_t *payload,
                                               uint32_t length)
{
    (void)payload;
    (void)length;

    if (event == WIFI_EVENT_IOT_SCAN_COMPLETE && s_scan_done_sem != NULL) {
        xSemaphoreGive(s_scan_done_sem);
    }

    return 0;
}

static void wifi_adapter_scan_register_handler(void)
{
    if (s_scan_handler_registered) {
        return;
    }

    s_scan_done_sem = xSemaphoreCreateBinary();
    if (s_scan_done_sem == NULL) {
        return;
    }

    if (wifi_connection_register_event_handler(WIFI_EVENT_IOT_SCAN_COMPLETE,
                                               wifi_adapter_scan_event_handler) < 0) {
        return;
    }

    s_scan_handler_registered = true;
}

size_t wifi_adapter_scan_networks(provision_scan_ap_t *out,
                                    size_t max_out,
                                    uint32_t timeout_ms)
{
    size_t count = 0;
    TickType_t deadline;

    if (out == NULL || max_out == 0) {
        return 0;
    }

    wifi_adapter_scan_register_handler();
    if (!s_scan_handler_registered || s_scan_done_sem == NULL) {
        return 0;
    }

    xSemaphoreTake(s_scan_done_sem, 0);

    if (wifi_connection_scan_init(s_scan_table, WIFI_ADAPTER_SCAN_TABLE_SIZE) < 0) {
        return 0;
    }

    if (wifi_connection_start_scan(NULL, 0, NULL, 1, 0) < 0) {
        wifi_connection_scan_deinit();
        return 0;
    }

    deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (xTaskGetTickCount() < deadline) {
        if (xSemaphoreTake(s_scan_done_sem, pdMS_TO_TICKS(200)) == pdTRUE) {
            break;
        }
    }

    wifi_connection_stop_scan();
    wifi_connection_scan_deinit();

    for (size_t i = 0; i < WIFI_ADAPTER_SCAN_TABLE_SIZE && count < max_out; i++) {
        const wifi_scan_list_item_t *item = &s_scan_table[i];
        size_t ssid_len;

        if (!item->is_valid || item->ssid_length == 0) {
            continue;
        }

        ssid_len = item->ssid_length;
        if (ssid_len > WIFI_SSID_MAX_LEN) {
            ssid_len = WIFI_SSID_MAX_LEN;
        }

        memcpy(out[count].ssid, item->ssid, ssid_len);
        out[count].ssid[ssid_len] = '\0';
        out[count].rssi = item->rssi;
        count++;
    }

    return count;
}
