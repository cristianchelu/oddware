/*
 * OTA pre-download memory reclaim — spec/30-processes/ota-flow.md
 */

#include "ota_preflight.h"

#include "FreeRTOS.h"
#include "task.h"

#include "app_log.h"
#include "app_cli_ota.h"
#include "mqtt_client.h"
#include "remote_cli.h"
#if WEB_UI_ENABLE
#include "web_ui.h"
#endif
#include "wifi_sta.h"

#define OTA_PREFLIGHT_MQTT_DISCONNECT_MS  5000u
#define OTA_PREFLIGHT_HEAP_SETTLE_MS       300u

port_err_t ota_preflight_suspend_idle_tasks(void)
{
    /*
     * Reclaim idle task stacks before xTaskCreate allocates the ~12 KB OTA
     * worker. Order: bench CLI, UART CLI, Wi-Fi connect worker, then MQTT.
     */
    remote_cli_suspend_for_ota();
#if WEB_UI_ENABLE
    web_ui_suspend_for_ota();
#endif
    app_cli_suspend_for_ota();
    wifi_sta_suspend_for_ota();

    mqtt_client_suspend_for_ota();

    if (!mqtt_client_wait_disconnected(OTA_PREFLIGHT_MQTT_DISCONNECT_MS)) {
        app_log_warn("ota", "mqtt disconnect timeout during preflight");
    }

    vTaskDelay(pdMS_TO_TICKS(OTA_PREFLIGHT_HEAP_SETTLE_MS));

    app_log_info("ota",
                 "preflight heap free=%u min=%u",
                 (unsigned)xPortGetFreeHeapSize(),
                 (unsigned)xPortGetMinimumEverFreeHeapSize());

    return PORT_OK;
}

void ota_preflight_resume_idle_tasks(void)
{
    wifi_sta_resume_after_ota();
    app_cli_resume_after_ota();
    remote_cli_resume_after_ota();
#if WEB_UI_ENABLE
    web_ui_resume_after_ota();
#endif
    mqtt_client_resume_after_ota();
}
