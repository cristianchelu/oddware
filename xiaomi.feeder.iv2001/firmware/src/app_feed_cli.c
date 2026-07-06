/*
 * UART CLI: feed command table — spec/30-processes/uart-console.md § feed
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_feed_cli.h"
#include "app_log.h"
#include "cli.h"
#include "feed_config.h"
#include "port_err.h"
#include "mqtt_feed_mode.h"
#include "mqtt_feed_overfill.h"

uint8_t feed_cli_run_mode_show(void)
{
    dispense_mode_t mode = feed_config_mode_get();

    app_log_info("cli", "feed mode: %s", feed_config_mode_string(mode));
    return 0;
}

uint8_t feed_cli_run_mode_set(dispense_mode_t mode)
{
    if (feed_config_mode_get() == mode) {
        app_log_info("cli", "feed mode: unchanged");
        return 1;
    }

    if (mqtt_feed_mode_apply(mode) != PORT_OK) {
        app_log_info("cli", "feed mode: nvdm write failed");
        return 1;
    }

    app_log_info("cli", "feed mode ok");
    return 0;
}

static uint8_t feed_cli_mode(uint8_t argc, char *argv[])
{
    if (argc < 1u || argv[0] == NULL) {
        return feed_cli_run_mode_show();
    }

    if (strcmp(argv[0], "open_loop") == 0) {
        return feed_cli_run_mode_set(DISPENSE_MODE_OPEN_LOOP);
    }

    if (strcmp(argv[0], "compensated") == 0) {
        return feed_cli_run_mode_set(DISPENSE_MODE_COMPENSATED);
    }

    app_log_info("cli", "usage: feed mode [open_loop|compensated]");
    return 1;
}

uint8_t feed_cli_run_overfill_show(void)
{
    app_log_info("cli",
                 "feed overfill: %s threshold_g=%u",
                 feed_config_overfill_enabled_get() ? "on" : "off",
                 (unsigned)feed_config_overfill_threshold_g_get());
    return 0;
}

uint8_t feed_cli_run_overfill_set(bool enabled)
{
    if (feed_config_overfill_enabled_get() == enabled) {
        app_log_info("cli", "feed overfill: unchanged");
        return 1;
    }

    if (!feed_config_overfill_enabled_set(enabled)) {
        app_log_info("cli", "feed overfill: nvdm write failed");
        return 1;
    }

    mqtt_feed_overfill_publish_snapshot();
    app_log_info("cli", "feed overfill ok");
    return 0;
}

static uint8_t feed_cli_overfill(uint8_t argc, char *argv[])
{
    if (argc < 1u || argv[0] == NULL) {
        return feed_cli_run_overfill_show();
    }

    if (strcmp(argv[0], "on") == 0) {
        return feed_cli_run_overfill_set(true);
    }

    if (strcmp(argv[0], "off") == 0) {
        return feed_cli_run_overfill_set(false);
    }

    app_log_info("cli", "usage: feed overfill [on|off]");
    return 1;
}

uint8_t feed_cli_run_overfill_g_show(void)
{
    app_log_info("cli",
                 "feed overfill_g: %u",
                 (unsigned)feed_config_overfill_threshold_g_get());
    return 0;
}

uint8_t feed_cli_run_overfill_g_set(uint8_t threshold_g)
{
    if (feed_config_overfill_threshold_g_get() == threshold_g) {
        app_log_info("cli", "feed overfill_g: unchanged");
        return 1;
    }

    if (!feed_config_overfill_threshold_g_set(threshold_g)) {
        app_log_info("cli", "feed overfill_g: nvdm write failed");
        return 1;
    }

    mqtt_feed_overfill_publish_snapshot();
    app_log_info("cli", "feed overfill_g ok");
    return 0;
}

static uint8_t feed_cli_overfill_g(uint8_t argc, char *argv[])
{
    unsigned long parsed;
    char *end = NULL;

    if (argc < 1u || argv[0] == NULL) {
        return feed_cli_run_overfill_g_show();
    }

    parsed = strtoul(argv[0], &end, 10);
    if (end == argv[0] || *end != '\0' || parsed < 30 || parsed > 100) {
        app_log_info("cli", "usage: feed overfill_g <30-100>");
        return 1;
    }

    return feed_cli_run_overfill_g_set((uint8_t)parsed);
}

cmd_t feed_cli_subcmds[] = {
    { "mode", "feed mode [open_loop|compensated]", feed_cli_mode, NULL },
    { "overfill", "feed overfill [on|off]", feed_cli_overfill, NULL },
    { "overfill_g", "feed overfill_g <30-100>", feed_cli_overfill_g, NULL },
    { NULL, NULL, NULL, NULL },
};
