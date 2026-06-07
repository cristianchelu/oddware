/*
 * Wi-Fi STA bring-up — spec/30-processes/uart-console.md (boot behavior)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "syslog.h"

#include "wifi_lwip_helper.h"

#include "config_port.h"
#include "wifi_cred.h"
#include "wifi_port.h"
#include "task_def.h"
#include "wifi_adapter.h"
#include "mqtt_client.h"
#include "wifi_sta.h"

log_create_module(wifi_sta, PRINT_LEVEL_INFO);

static TaskHandle_t s_connect_task;
static volatile bool s_connect_busy;

static void wifi_sta_apply_connect(const char *ssid, const char *pass)
{
    const wifi_port_t *wifi = wifi_port_get();

    LOG_I(wifi_sta, "connecting to \"%s\"", ssid);

    if (wifi->connect(ssid, pass) != PORT_OK) {
        LOG_E(wifi_sta, "connect failed");
        return;
    }

    lwip_net_ready();

    {
        char ip[20];

        if (wifi->get_ip(ip, sizeof(ip)) == PORT_OK) {
            LOG_I(wifi_sta, "STA ready, IP %s", ip);
            mqtt_client_notify_wifi_ready();
        }
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
            LOG_E(wifi_sta, "no valid credentials in NVDM");
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
        LOG_E(wifi_sta, "failed to start connect task");
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
