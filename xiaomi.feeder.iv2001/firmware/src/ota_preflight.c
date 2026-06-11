/*
 * OTA pre-download memory reclaim — spec/30-processes/ota-flow.md
 */

#include "ota_preflight.h"

#include "app_log.h"
#include "mqtt_client.h"
#include "remote_cli.h"

#define OTA_PREFLIGHT_MQTT_DISCONNECT_MS  2000u

port_err_t ota_preflight_suspend_idle_tasks(void)
{
    mqtt_client_suspend_for_ota();

    if (!mqtt_client_wait_disconnected(OTA_PREFLIGHT_MQTT_DISCONNECT_MS)) {
        app_log_warn("ota", "mqtt disconnect timeout during preflight");
    }

    remote_cli_suspend_for_ota();

    return PORT_OK;
}

void ota_preflight_resume_idle_tasks(void)
{
    remote_cli_resume_after_ota();
    mqtt_client_resume_after_ota();
}
