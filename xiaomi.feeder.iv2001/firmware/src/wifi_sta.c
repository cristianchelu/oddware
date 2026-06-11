/*
 * Wi-Fi STA bring-up — spec/30-processes/uart-console.md (boot behavior)
 */

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "wifi_lwip_helper.h"

#include "app_log.h"
#include "app_event.h"
#include "config_port.h"
#include "wifi_cred.h"
#include "wifi_port.h"
#include "task_def.h"
#include "wifi_adapter.h"
#include "wifi_sta.h"

#define WIFI_STA_LINK_TIMEOUT_MS 60000u
#define WIFI_STA_DHCP_TIMEOUT_MS 60000u

/* Captive-portal AP static address — not a valid STA DHCP lease. */
#define WIFI_STA_AP_LEFTOVER_IP "192.168.4.1"

static TaskHandle_t s_connect_task;
static volatile bool s_connect_busy;

static void wifi_sta_post(app_event_type_t type)
{
    app_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    (void)app_event_post(&ev);
}

static bool wifi_sta_ip_is_valid_lease(const char *ip)
{
    return ip != NULL && ip[0] != '\0' && strcmp(ip, WIFI_STA_AP_LEFTOVER_IP) != 0;
}

static void wifi_sta_apply_connect(const char *ssid, const char *pass)
{
    const wifi_port_t *wifi = wifi_port_get();
    char ip[20];
    app_event_t ev;

    wifi_sta_post(EVT_WIFI_STA_CONNECTING);

    if (wifi->connect(ssid, pass) != PORT_OK) {
        wifi_sta_post(EVT_WIFI_STA_FAILED);
        return;
    }

    if (!lwip_net_link_ready_timeout(WIFI_STA_LINK_TIMEOUT_MS)) {
        APP_LOG_E("wifi", "connect failed");
        wifi_sta_post(EVT_WIFI_STA_FAILED);
        return;
    }

    APP_LOG_I("wifi", "associated");

    if (!lwip_net_dhcp_ready_timeout(WIFI_STA_DHCP_TIMEOUT_MS)) {
        APP_LOG_E("wifi", "connect failed");
        wifi_sta_post(EVT_WIFI_STA_FAILED);
        return;
    }

    memset(&ev, 0, sizeof(ev));
    if (wifi->get_ip(ip, sizeof(ip)) == PORT_OK && wifi_sta_ip_is_valid_lease(ip)) {
        wifi_adapter_log_sta_dhcp_ready(ip);
        ev.type = EVT_WIFI_STA_READY;
        strncpy(ev.u.wifi_ready.ip, ip, sizeof(ev.u.wifi_ready.ip) - 1);
        (void)app_event_post(&ev);
    } else {
        APP_LOG_E("wifi", "connect failed");
        wifi_sta_post(EVT_WIFI_STA_FAILED);
    }
}

static void wifi_sta_connect_task(void *param)
{
    char ssid[WIFI_SSID_MAX_LEN + 1];
    char pass[WIFI_PASS_MAX_LEN + 1];

    (void)param;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (wifi_cred_load(config_port_get(),
                           ssid, sizeof(ssid),
                           pass, sizeof(pass)) != PORT_OK) {
            APP_LOG_E("wifi", "no valid credentials in NVDM");
            s_connect_busy = false;
            continue;
        }

        wifi_sta_apply_connect(ssid, pass);
        s_connect_busy = false;
    }
}

void wifi_sta_start(void)
{
    wifi_adapter_stack_init();

    if (xTaskCreate(wifi_sta_connect_task,
                    "wifi_sta",
                    APP_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                    NULL,
                    APP_TASK_PRIO,
                    &s_connect_task) != pdPASS) {
        APP_LOG_E("wifi", "failed to start connect task");
        return;
    }

    if (wifi_cred_is_stored(config_port_get())) {
        wifi_sta_request_connect();
    }
}

bool wifi_sta_request_connect(void)
{
    if (s_connect_task == NULL) {
        return false;
    }

    if (s_connect_busy) {
        return false;
    }

    s_connect_busy = true;
    xTaskNotifyGive(s_connect_task);
    return true;
}
