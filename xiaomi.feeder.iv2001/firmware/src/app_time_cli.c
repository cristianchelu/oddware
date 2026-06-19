/*
 * UART CLI: time commands — spec/30-processes/uart-console.md
 */

#include <stdio.h>

#include "app_log.h"
#include "app_time_cli.h"
#include "cli.h"
#include "time_local.h"
#include "time_sync.h"
#include "tz_rule.h"

#include "config_port.h"

uint8_t time_cli_run_show(void)
{
    tz_rule_t rule;
    char wire[TZ_RULE_WIRE_MAX];
    time_local_t local;
    int64_t utc = 0;

    if (tz_rule_load(config_port_get(), &rule) != PORT_OK) {
        tz_rule_default(&rule);
    }

    if (!tz_rule_format_wire(&rule, wire, sizeof(wire))) {
        app_log_info("cli", "time: tz error");
        return 1;
    }

    if (!time_sync_is_valid() || !time_sync_get_utc_epoch(&utc) || !time_local_now(&local)) {
        app_log_info("cli", "time: not synced tz_rule=%s", wire);
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
                 wire);
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
    { NULL, NULL, NULL, NULL },
};
