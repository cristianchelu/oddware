/*
 * UART CLI: time commands — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "app_time_cli.h"
#include "cli.h"
#include "time_config.h"
#include "time_local.h"
#include "time_sync.h"
#include "tz_rule.h"

#include "config_port.h"

uint8_t time_cli_run_show(void)
{
    char posix[TZ_RULE_POSIX_MAX];
    time_local_t local;
    int64_t utc = 0;

    if (tz_rule_load_posix(config_port_get(), posix, sizeof(posix)) != PORT_OK) {
        strcpy(posix, "UTC0");
    }

    if (!time_sync_is_valid() || !time_sync_get_utc_epoch(&utc) || !time_local_now(&local)) {
        app_log_info("cli", "time: not synced tz_rule=%s", posix);
        return 0;
    }

    app_log_info("cli",
                 "time: synced utc=%lu local=%04u-%02u-%02u %02u:%02u:%02u wday=%u tz_rule=%s",
                 (unsigned long)utc,
                 (unsigned)local.year,
                 (unsigned)local.month,
                 (unsigned)local.day,
                 (unsigned)local.hour,
                 (unsigned)local.min,
                 (unsigned)local.sec,
                 (unsigned)local.wday_mon0,
                 posix);
    return 0;
}

uint8_t time_cli_run_sync(void)
{
    switch (time_sync_request_now()) {
    case TIME_SYNC_REQUEST_OK:
        app_log_info("cli", "time sync started");
        return 0;
    case TIME_SYNC_REQUEST_NO_NETWORK:
        app_log_info("cli", "time sync: no network");
        return 1;
    case TIME_SYNC_REQUEST_BUSY:
    default:
        app_log_info("cli", "time sync busy");
        return 1;
    }
}

uint8_t time_cli_run_set_tz_rule(const char *posix)
{
    time_config_patch_t patch;
    port_err_t err;

    if (posix == NULL || posix[0] == '\0') {
        app_log_info("cli", "usage: time set tz_rule <posix>");
        return 1;
    }

    patch.tz_rule_posix = posix;
    patch.tz_label = NULL;
    err = time_config_apply(config_port_get(), &patch);
    if (err == PORT_OK) {
        app_log_info("cli", "tz_rule saved");
        return 0;
    }
    if (err == PORT_ERR_IO) {
        app_log_info("cli", "nvdm write failed");
        return 1;
    }

    app_log_info("cli", "invalid tz_rule");
    return 1;
}

uint8_t time_cli_run_set_tz_label(const char *label)
{
    time_config_patch_t patch;
    port_err_t err;

    if (label == NULL || label[0] == '\0') {
        app_log_info("cli", "usage: time set tz_label <name>");
        return 1;
    }

    patch.tz_rule_posix = NULL;
    patch.tz_label = label;
    err = time_config_apply(config_port_get(), &patch);
    if (err == PORT_OK) {
        app_log_info("cli", "tz_label saved");
        return 0;
    }
    if (err == PORT_ERR_IO) {
        app_log_info("cli", "nvdm write failed");
        return 1;
    }

    app_log_info("cli", "invalid tz_label");
    return 1;
}

static uint8_t time_cli_set_tz_rule(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: time set tz_rule <posix>");
        return 1;
    }

    return time_cli_run_set_tz_rule(argv[0]);
}

static uint8_t time_cli_set_tz_label(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: time set tz_label <name>");
        return 1;
    }

    return time_cli_run_set_tz_label(argv[0]);
}

static uint8_t time_cli_set(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: time set tz_rule|tz_label <value>");
        return 1;
    }

    if (strcmp(argv[0], "tz_rule") == 0) {
        return time_cli_set_tz_rule(argc - 1, argv + 1);
    }
    if (strcmp(argv[0], "tz_label") == 0) {
        return time_cli_set_tz_label(argc - 1, argv + 1);
    }

    app_log_info("cli", "usage: time set tz_rule|tz_label <value>");
    return 1;
}

static uint8_t time_cli_show(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return time_cli_run_show();
}

static uint8_t time_cli_sync(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return time_cli_run_sync();
}

cmd_t time_cli_subcmds[] = {
    { "show", "time show", time_cli_show, NULL },
    { "sync", "request NTP sync", time_cli_sync, NULL },
    { "set",  "set tz_rule|tz_label", time_cli_set, NULL },
    { NULL, NULL, NULL, NULL },
};
