/*
 * Application entry — spec/40-architecture/build-integration.md
 */

#include "FreeRTOS.h"
#include "task.h"
#include "sys_init.h"
#include "syslog.h"
#include "system_mt7682.h"

#include "app.h"
#include "app_cli.h"
#include "boot_bank_target.h"
#include "ota_client.h"
#include "ota_slot_health.h"
#include "config_port.h"
#include "provision.h"
#include "wifi_cred.h"
#include "wifi_sta.h"
#include "motor_ctrl.h"
#include "mqtt_client.h"

log_create_module(petfeeder, PRINT_LEVEL_INFO);

int main(void)
{
    system_init();

    LOG_I(petfeeder, "FreeRTOS Running (bank %c)",
          (boot_bank_query_active() == BOOT_BANK_B) ? 'B' : 'A');

    app_start();
    motor_ctrl_start();
    app_cli_start();
    wifi_sta_start();
    provision_start();
    ota_slot_health_on_boot();
    ota_client_start();
    if (wifi_cred_is_stored(config_port_get())) {
        mqtt_client_start();
    }

    SysInitStatus_Set();
    vTaskStartScheduler();

    for (;;) {
    }
}
