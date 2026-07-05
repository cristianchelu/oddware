/*
 * UART CLI: schedule commands — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_log.h"
#include "app_schedule_cli.h"
#include "cli.h"
#include "schedule.h"
#include "schedule_cmd.h"

static bool schedule_cli_parse_on_off(const char *word, bool *out)
{
    if (word == NULL || out == NULL) {
        return false;
    }

    if (strcmp(word, "on") == 0) {
        *out = true;
        return true;
    }

    if (strcmp(word, "off") == 0) {
        *out = false;
        return true;
    }

    return false;
}

static bool schedule_cli_parse_uint(const char *word, unsigned max, unsigned *out)
{
    char *end = NULL;
    unsigned long value;

    if (word == NULL || out == NULL || word[0] == '\0') {
        return false;
    }

    value = strtoul(word, &end, 10);
    if (end == word || *end != '\0' || value > max) {
        return false;
    }

    *out = (unsigned)value;
    return true;
}

uint8_t schedule_cli_run_show(void)
{
    size_t count = schedule_slot_count();
    size_t i;

    app_log_info("cli",
                 "schedule: enabled=%s today=%s slots=%u",
                 schedule_global_enabled() ? "on" : "off",
                 schedule_today_enabled() ? "on" : "off",
                 (unsigned)count);

    for (i = 0; i < count; i++) {
        schedule_slot_config_t cfg;
        schedule_slot_runtime_t rt;
        char g_actual_buf[8];

        if (!schedule_get_slot(i, &cfg, &rt)) {
            continue;
        }

        if (rt.g_actual >= 0) {
            snprintf(g_actual_buf, sizeof(g_actual_buf), "%d", (int)rt.g_actual);
        } else {
            g_actual_buf[0] = '-';
            g_actual_buf[1] = '\0';
        }

        app_log_info("cli",
                     "schedule: %02u:%02u days=%u g=%u enabled=%s state=%s skip=%s g_actual=%s",
                     (unsigned)cfg.hour,
                     (unsigned)cfg.min,
                     (unsigned)cfg.days,
                     (unsigned)cfg.g,
                     cfg.enabled ? "on" : "off",
                     schedule_state_wire(rt.state),
                     rt.skip_today ? "on" : "off",
                     g_actual_buf);
    }

    return 0;
}

uint8_t schedule_cli_run_next(void)
{
    schedule_next_t next;

    if (!schedule_compute_next(&next)) {
        app_log_info("cli", "schedule: no upcoming");
        return 0;
    }

    app_log_info("cli",
                 "schedule next: %02u:%02u %ug in %ld min",
                 (unsigned)next.hour,
                 (unsigned)next.min,
                 (unsigned)next.g,
                 (long)next.in_min);
    return 0;
}

uint8_t schedule_cli_run_set(unsigned hour,
                             unsigned min,
                             unsigned days,
                             unsigned g,
                             bool enabled)
{
    schedule_slot_config_t cfg;
    schedule_cmd_result_t result;

    cfg.hour = (uint8_t)hour;
    cfg.min = (uint8_t)min;
    cfg.days = (uint8_t)days;
    cfg.g = (uint8_t)g;
    cfg.enabled = enabled;

    result = schedule_cmd_set(&cfg);
    switch (result) {
    case SCHEDULE_CMD_OK:
        app_log_info("cli", "schedule set ok");
        return 0;
    case SCHEDULE_CMD_INVALID:
        if (hour > 23u) {
            app_log_info("cli", "schedule: invalid hour");
        } else if (min > 59u) {
            app_log_info("cli", "schedule: invalid min");
        } else if (days > 127u) {
            app_log_info("cli", "schedule: invalid days");
        } else {
            app_log_info("cli", "schedule: invalid g");
        }
        return 1;
    case SCHEDULE_CMD_NVDM_FAIL:
        app_log_info("cli", "schedule: nvdm write failed");
        return 1;
    default:
        return 1;
    }
}

uint8_t schedule_cli_run_delete(unsigned hour, unsigned min)
{
    schedule_cmd_result_t result = schedule_cmd_delete((uint8_t)hour, (uint8_t)min);

    switch (result) {
    case SCHEDULE_CMD_OK:
        app_log_info("cli", "schedule delete ok");
        return 0;
    case SCHEDULE_CMD_INVALID:
        app_log_info("cli", "schedule: invalid time");
        return 1;
    case SCHEDULE_CMD_NOT_FOUND:
        app_log_info("cli", "schedule: slot not found");
        return 1;
    case SCHEDULE_CMD_NVDM_FAIL:
        app_log_info("cli", "schedule: nvdm write failed");
        return 1;
    default:
        return 1;
    }
}

uint8_t schedule_cli_run_toggle(unsigned hour, unsigned min)
{
    schedule_cmd_result_t result = schedule_cmd_toggle((uint8_t)hour, (uint8_t)min);

    switch (result) {
    case SCHEDULE_CMD_OK:
        app_log_info("cli", "schedule toggle ok");
        return 0;
    case SCHEDULE_CMD_INVALID:
        app_log_info("cli", "schedule: invalid time");
        return 1;
    case SCHEDULE_CMD_NOT_FOUND:
        app_log_info("cli", "schedule: slot not found");
        return 1;
    case SCHEDULE_CMD_NVDM_FAIL:
        app_log_info("cli", "schedule: nvdm write failed");
        return 1;
    default:
        return 1;
    }
}

uint8_t schedule_cli_run_skip(unsigned hour, unsigned min, bool skip)
{
    schedule_cmd_result_t result = schedule_cmd_skip((uint8_t)hour, (uint8_t)min, skip);

    switch (result) {
    case SCHEDULE_CMD_OK:
        app_log_info("cli", "schedule skip ok");
        return 0;
    case SCHEDULE_CMD_INVALID:
        app_log_info("cli", "schedule: invalid time");
        return 1;
    case SCHEDULE_CMD_NOT_FOUND:
        app_log_info("cli", "schedule: slot not found");
        return 1;
    default:
        return 1;
    }
}

uint8_t schedule_cli_run_enable(bool enabled)
{
    schedule_cmd_result_t result = schedule_cmd_enable(enabled);

    switch (result) {
    case SCHEDULE_CMD_OK:
        app_log_info("cli", "schedule enable ok");
        return 0;
    case SCHEDULE_CMD_UNCHANGED:
        app_log_info("cli", "schedule: unchanged");
        return 1;
    case SCHEDULE_CMD_NVDM_FAIL:
        app_log_info("cli", "schedule: nvdm write failed");
        return 1;
    default:
        return 1;
    }
}

uint8_t schedule_cli_run_today(bool enabled)
{
    schedule_cmd_result_t result = schedule_cmd_today(enabled);

    switch (result) {
    case SCHEDULE_CMD_OK:
        app_log_info("cli", "schedule today ok");
        return 0;
    case SCHEDULE_CMD_UNCHANGED:
        app_log_info("cli", "schedule: unchanged");
        return 1;
    case SCHEDULE_CMD_NVDM_FAIL:
        app_log_info("cli", "schedule: nvdm write failed");
        return 1;
    default:
        return 1;
    }
}

static uint8_t schedule_cli_show(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return schedule_cli_run_show();
}

static uint8_t schedule_cli_next(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return schedule_cli_run_next();
}

static uint8_t schedule_cli_set(uint8_t argc, char *argv[])
{
    unsigned hour = 0;
    unsigned min = 0;
    unsigned days = 0;
    unsigned g = 0;
    bool enabled = true;

    if (argc < 4) {
        app_log_info("cli", "usage: schedule set <hour> <min> <days> <g> [on|off]");
        return 1;
    }

    if (!schedule_cli_parse_uint(argv[0], 23u, &hour)) {
        app_log_info("cli", "schedule: invalid hour");
        return 1;
    }

    if (!schedule_cli_parse_uint(argv[1], 59u, &min)) {
        app_log_info("cli", "schedule: invalid min");
        return 1;
    }

    if (!schedule_cli_parse_uint(argv[2], 127u, &days)) {
        app_log_info("cli", "schedule: invalid days");
        return 1;
    }

    if (!schedule_cli_parse_uint(argv[3], SCHEDULE_G_MAX, &g) || g < SCHEDULE_G_MIN) {
        app_log_info("cli", "schedule: invalid g");
        return 1;
    }

    if (argc >= 5) {
        if (!schedule_cli_parse_on_off(argv[4], &enabled)) {
            app_log_info("cli", "usage: schedule set <hour> <min> <days> <g> [on|off]");
            return 1;
        }
    }

    return schedule_cli_run_set(hour, min, days, g, enabled);
}

static uint8_t schedule_cli_delete(uint8_t argc, char *argv[])
{
    unsigned hour = 0;
    unsigned min = 0;

    if (argc < 2) {
        app_log_info("cli", "usage: schedule delete <hour> <min>");
        return 1;
    }

    if (!schedule_cli_parse_uint(argv[0], 23u, &hour) ||
        !schedule_cli_parse_uint(argv[1], 59u, &min)) {
        app_log_info("cli", "schedule: invalid time");
        return 1;
    }

    return schedule_cli_run_delete(hour, min);
}

static uint8_t schedule_cli_toggle(uint8_t argc, char *argv[])
{
    unsigned hour = 0;
    unsigned min = 0;

    if (argc < 2) {
        app_log_info("cli", "usage: schedule toggle <hour> <min>");
        return 1;
    }

    if (!schedule_cli_parse_uint(argv[0], 23u, &hour) ||
        !schedule_cli_parse_uint(argv[1], 59u, &min)) {
        app_log_info("cli", "schedule: invalid time");
        return 1;
    }

    return schedule_cli_run_toggle(hour, min);
}

static uint8_t schedule_cli_skip(uint8_t argc, char *argv[])
{
    unsigned hour = 0;
    unsigned min = 0;
    bool skip = false;

    if (argc < 3) {
        app_log_info("cli", "usage: schedule skip <hour> <min> on|off");
        return 1;
    }

    if (!schedule_cli_parse_uint(argv[0], 23u, &hour) ||
        !schedule_cli_parse_uint(argv[1], 59u, &min) ||
        !schedule_cli_parse_on_off(argv[2], &skip)) {
        app_log_info("cli", "usage: schedule skip <hour> <min> on|off");
        return 1;
    }

    return schedule_cli_run_skip(hour, min, skip);
}

static uint8_t schedule_cli_enable(uint8_t argc, char *argv[])
{
    bool enabled = false;

    if (argc < 1 || !schedule_cli_parse_on_off(argv[0], &enabled)) {
        app_log_info("cli", "usage: schedule enable on|off");
        return 1;
    }

    return schedule_cli_run_enable(enabled);
}

static uint8_t schedule_cli_today(uint8_t argc, char *argv[])
{
    bool enabled = false;

    if (argc < 1 || !schedule_cli_parse_on_off(argv[0], &enabled)) {
        app_log_info("cli", "usage: schedule today on|off");
        return 1;
    }

    return schedule_cli_run_today(enabled);
}

cmd_t schedule_cli_subcmds[] = {
    { "show",   "schedule show", schedule_cli_show, NULL },
    { "next",   "schedule next", schedule_cli_next, NULL },
    { "set",    "schedule set", schedule_cli_set, NULL },
    { "delete", "schedule delete", schedule_cli_delete, NULL },
    { "toggle", "schedule toggle", schedule_cli_toggle, NULL },
    { "skip",   "schedule skip", schedule_cli_skip, NULL },
    { "enable", "schedule enable", schedule_cli_enable, NULL },
    { "today",  "schedule today", schedule_cli_today, NULL },
    { NULL, NULL, NULL, NULL },
};
