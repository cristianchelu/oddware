/*
 * Schedule command layer — spec/30-processes/web-ui.md, scheduler-engine.md
 */

#include "schedule_cmd.h"

#include <stdio.h>
#include <string.h>

#include "mqtt_json.h"
#include "schedule.h"

static bool schedule_cmd_slot_exists(uint8_t hour, uint8_t min)
{
    size_t i;

    for (i = 0; i < schedule_slot_count(); i++) {
        schedule_slot_config_t cfg;

        if (schedule_get_slot(i, &cfg, NULL) && cfg.hour == hour && cfg.min == min) {
            return true;
        }
    }

    return false;
}

static bool schedule_cmd_json_has_legacy_days(const char *json, size_t len)
{
    const char *cursor;

    if (json == NULL || len == 0) {
        return false;
    }

    cursor = json;
    while (cursor < json + len) {
        const char *hit = strstr(cursor, "\"days\":");

        if (hit == NULL || hit >= json + len) {
            return false;
        }

        if (hit == json || hit[-1] != '_') {
            return true;
        }

        cursor = hit + 1;
    }

    return false;
}

static bool schedule_cmd_json_find_repeat_days(const char *json,
                                               size_t len,
                                               uint8_t *days_out)
{
    const char *key = "\"repeat_days\":";
    const char *cursor;
    const char *end;
    uint8_t days = 0;

    if (json == NULL || days_out == NULL || len == 0) {
        return false;
    }

    cursor = strstr(json, key);
    if (cursor == NULL || cursor >= json + len) {
        return false;
    }

    cursor += strlen(key);
    end = json + len;

    while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
        cursor++;
    }

    if (cursor >= end || *cursor != '[') {
        return false;
    }

    cursor++;

    while (cursor < end) {
        unsigned value = 0;

        while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
            cursor++;
        }

        if (cursor < end && *cursor == ']') {
            break;
        }

        if (sscanf(cursor, "%u", &value) != 1 || value > 6u) {
            return false;
        }

        days = (uint8_t)(days | (uint8_t)(1u << value));

        while (cursor < end && *cursor != ',' && *cursor != ']') {
            cursor++;
        }

        if (cursor < end && *cursor == ',') {
            cursor++;
        }
    }

    *days_out = days;
    return true;
}

schedule_cmd_result_t schedule_cmd_set(const schedule_slot_config_t *cfg)
{
    if (cfg == NULL || cfg->hour > 23u || cfg->min > 59u || cfg->days > 127u ||
        cfg->g < SCHEDULE_G_MIN || cfg->g > SCHEDULE_G_MAX) {
        return SCHEDULE_CMD_INVALID;
    }

    if (!schedule_set_slot(cfg)) {
        return SCHEDULE_CMD_NVDM_FAIL;
    }

    return SCHEDULE_CMD_OK;
}

schedule_cmd_result_t schedule_cmd_delete(uint8_t hour, uint8_t min)
{
    if (hour > 23u || min > 59u) {
        return SCHEDULE_CMD_INVALID;
    }

    if (!schedule_delete_slot(hour, min)) {
        if (schedule_cmd_slot_exists(hour, min)) {
            return SCHEDULE_CMD_NVDM_FAIL;
        }

        return SCHEDULE_CMD_NOT_FOUND;
    }

    return SCHEDULE_CMD_OK;
}

schedule_cmd_result_t schedule_cmd_toggle(uint8_t hour, uint8_t min)
{
    if (hour > 23u || min > 59u) {
        return SCHEDULE_CMD_INVALID;
    }

    if (!schedule_toggle_slot(hour, min)) {
        if (schedule_cmd_slot_exists(hour, min)) {
            return SCHEDULE_CMD_NVDM_FAIL;
        }

        return SCHEDULE_CMD_NOT_FOUND;
    }

    return SCHEDULE_CMD_OK;
}

schedule_cmd_result_t schedule_cmd_skip(uint8_t hour, uint8_t min, bool skip)
{
    if (hour > 23u || min > 59u) {
        return SCHEDULE_CMD_INVALID;
    }

    if (!schedule_skip_slot(hour, min, skip)) {
        return SCHEDULE_CMD_NOT_FOUND;
    }

    return SCHEDULE_CMD_OK;
}

schedule_cmd_result_t schedule_cmd_enable(bool enabled)
{
    if (schedule_global_enabled() == enabled) {
        return SCHEDULE_CMD_UNCHANGED;
    }

    if (!schedule_set_global_enabled(enabled)) {
        return SCHEDULE_CMD_NVDM_FAIL;
    }

    return SCHEDULE_CMD_OK;
}

schedule_cmd_result_t schedule_cmd_today(bool enabled)
{
    if (schedule_today_enabled() == enabled) {
        return SCHEDULE_CMD_UNCHANGED;
    }

    if (!schedule_set_today_enabled(enabled)) {
        return SCHEDULE_CMD_NVDM_FAIL;
    }

    return SCHEDULE_CMD_OK;
}

bool schedule_cmd_apply_json(mqtt_route_kind_t route,
                             const char *json,
                             size_t len)
{
    schedule_slot_config_t cfg;
    unsigned hour = 0;
    unsigned min = 0;
    unsigned g = 0;
    bool enabled = true;
    bool skip = true;
    schedule_cmd_result_t result;

    if (json == NULL || len == 0) {
        return false;
    }

    switch (route) {
    case MQTT_ROUTE_CMD_SCHEDULE_SET:
        if (schedule_cmd_json_has_legacy_days(json, len)) {
            return false;
        }

        if (!mqtt_json_find_uint(json, len, "hour", &hour) ||
            !mqtt_json_find_uint(json, len, "min", &min) ||
            !schedule_cmd_json_find_repeat_days(json, len, &cfg.days) ||
            !mqtt_json_find_uint(json, len, "g", &g)) {
            return false;
        }

        if (!mqtt_json_find_bool(json, len, "enabled", &enabled)) {
            enabled = true;
        }

        cfg.hour = (uint8_t)hour;
        cfg.min = (uint8_t)min;
        cfg.g = (uint8_t)g;
        cfg.enabled = enabled;
        result = schedule_cmd_set(&cfg);
        return result == SCHEDULE_CMD_OK;

    case MQTT_ROUTE_CMD_SCHEDULE_DELETE:
        if (!mqtt_json_find_uint(json, len, "hour", &hour) ||
            !mqtt_json_find_uint(json, len, "min", &min)) {
            return false;
        }

        result = schedule_cmd_delete((uint8_t)hour, (uint8_t)min);
        return result == SCHEDULE_CMD_OK;

    case MQTT_ROUTE_CMD_SCHEDULE_TOGGLE:
        if (!mqtt_json_find_uint(json, len, "hour", &hour) ||
            !mqtt_json_find_uint(json, len, "min", &min)) {
            return false;
        }

        result = schedule_cmd_toggle((uint8_t)hour, (uint8_t)min);
        return result == SCHEDULE_CMD_OK;

    case MQTT_ROUTE_CMD_SCHEDULE_SKIP:
        if (!mqtt_json_find_uint(json, len, "hour", &hour) ||
            !mqtt_json_find_uint(json, len, "min", &min) ||
            !mqtt_json_find_bool(json, len, "skip", &skip)) {
            return false;
        }

        result = schedule_cmd_skip((uint8_t)hour, (uint8_t)min, skip);
        return result == SCHEDULE_CMD_OK;

    case MQTT_ROUTE_CMD_SCHEDULE_ENABLE:
        if (!mqtt_json_find_bool(json, len, "enabled", &enabled)) {
            return false;
        }

        result = schedule_cmd_enable(enabled);
        return result == SCHEDULE_CMD_OK || result == SCHEDULE_CMD_UNCHANGED;

    case MQTT_ROUTE_CMD_SCHEDULE_TODAY:
        if (!mqtt_json_find_bool(json, len, "enabled", &enabled)) {
            return false;
        }

        result = schedule_cmd_today(enabled);
        return result == SCHEDULE_CMD_OK || result == SCHEDULE_CMD_UNCHANGED;

    default:
        return false;
    }
}
