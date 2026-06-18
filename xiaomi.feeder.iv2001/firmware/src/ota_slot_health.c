/*
 * Slot health confirm after bank swap — spec/30-processes/ota-flow.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "app_log.h"
#include "boot_bank_target.h"
#include "config_keys.h"
#include "config_port.h"

#define OTA_SLOT_CONFIRM_MS 60000u

static bool s_unverified;
static bool s_timer_running;
static TickType_t s_deadline;

static void ota_slot_health_increment_boot_count(const config_port_t *cfg)
{
    char buf[16];
    unsigned long count = 0;
    char *end;

    if (cfg->read(CONFIG_GROUP_SYSTEM, CONFIG_KEY_BOOT_COUNT, buf, sizeof(buf)) == PORT_OK) {
        count = strtoul(buf, &end, 10);
        if (end == buf) {
            count = 0;
        }
    }

    snprintf(buf, sizeof(buf), "%lu", count + 1u);
    cfg->write(CONFIG_GROUP_SYSTEM, CONFIG_KEY_BOOT_COUNT, buf);
}

static void ota_slot_health_clear_boot_count(const config_port_t *cfg)
{
    cfg->write(CONFIG_GROUP_SYSTEM, CONFIG_KEY_BOOT_COUNT, "0");
}

void ota_slot_health_on_boot(void)
{
    const config_port_t *cfg = config_port_get();

    s_unverified = boot_bank_query_unverified();
    s_timer_running = false;
    s_deadline = 0;

    if (!s_unverified) {
        return;
    }

    ota_slot_health_increment_boot_count(cfg);
    s_timer_running = true;
    s_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(OTA_SLOT_CONFIRM_MS);
    app_log_info("ota", "unverified slot — confirm timer started");
}

uint32_t ota_slot_health_poll_ms(void)
{
    if (!s_timer_running) {
        return 0;
    }

    if (xTaskGetTickCount() >= s_deadline) {
        const config_port_t *cfg = config_port_get();

        s_timer_running = false;
        if (boot_bank_confirm_boot() != 0) {
            app_log_error("ota", "slot confirm failed");
            return 0;
        }

        ota_slot_health_clear_boot_count(cfg);
        s_unverified = false;
        app_log_info("ota", "slot health confirmed");
        return 0;
    }

    return 1000;
}
