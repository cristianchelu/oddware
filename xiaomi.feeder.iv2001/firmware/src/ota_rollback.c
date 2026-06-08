/*
 * OTA rollback after failed post-update boot — spec/30-processes/ota-flow.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "syslog.h"

#include "boot_bank_target.h"
#include "config_keys.h"
#include "config_port.h"
#include "hal_cache.h"
#include "hal_sys.h"

log_create_module(ota_rollback, PRINT_LEVEL_INFO);

#define OTA_ROLLBACK_TIMEOUT_MS 60000u

static bool s_pending;
static bool s_timer_running;
static TickType_t s_deadline;
static bool s_rollback_triggered;

static bool ota_rollback_read_flag(const config_port_t *cfg)
{
    char buf[8];

    if (cfg->read(CONFIG_GROUP_SYSTEM, CONFIG_KEY_OTA_PENDING, buf, sizeof(buf)) != PORT_OK) {
        return false;
    }

    return strcmp(buf, "1") == 0;
}

static void ota_rollback_write_flag(const config_port_t *cfg, bool pending)
{
    cfg->write(CONFIG_GROUP_SYSTEM, CONFIG_KEY_OTA_PENDING, pending ? "1" : "0");
}

static void ota_rollback_increment_boot_count(const config_port_t *cfg)
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

static void ota_rollback_clear_boot_count(const config_port_t *cfg)
{
    cfg->write(CONFIG_GROUP_SYSTEM, CONFIG_KEY_BOOT_COUNT, "0");
}

static void ota_rollback_do_revert(void)
{
    if (boot_bank_switch_active() != 0) {
        LOG_E(ota_rollback, "rollback bank switch failed");
        return;
    }

    LOG_W(ota_rollback, "rollback — reverting to previous bank");
    vTaskDelay(pdMS_TO_TICKS(200));
    hal_cache_disable();
    hal_cache_deinit();
    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
}

void ota_rollback_mark_pending(void)
{
    const config_port_t *cfg = config_port_get();

    ota_rollback_write_flag(cfg, true);
}

void ota_rollback_on_boot(void)
{
    const config_port_t *cfg = config_port_get();

    s_pending = ota_rollback_read_flag(cfg);
    s_timer_running = false;
    s_rollback_triggered = false;
    s_deadline = 0;

    if (!s_pending) {
        return;
    }

    ota_rollback_increment_boot_count(cfg);
    s_timer_running = true;
    s_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(OTA_ROLLBACK_TIMEOUT_MS);
    LOG_I(ota_rollback, "post-OTA boot — rollback timer started");
}

void ota_rollback_on_mqtt_connected(void)
{
    const config_port_t *cfg = config_port_get();

    if (!s_pending) {
        return;
    }

    ota_rollback_write_flag(cfg, false);
    ota_rollback_clear_boot_count(cfg);
    s_pending = false;
    s_timer_running = false;
    LOG_I(ota_rollback, "post-OTA health check passed");
}

uint32_t ota_rollback_poll_ms(void)
{
    if (!s_timer_running || s_rollback_triggered) {
        return 0;
    }

    if (xTaskGetTickCount() >= s_deadline) {
        const config_port_t *cfg = config_port_get();

        s_rollback_triggered = true;
        s_timer_running = false;
        ota_rollback_write_flag(cfg, false);
        ota_rollback_do_revert();
        return 0;
    }

    return 1000;
}
