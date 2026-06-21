/*
 * Schedule MQTT publisher — spec/30-processes/mqtt-protocol.md § Schedule
 */

#include "mqtt_schedule.h"

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "mqtt_client.h"
#include "mqtt_topics.h"
#include "port_err.h"
#include "schedule.h"

#define MQTT_SCHEDULE_PAYLOAD_MAX 4096u

static char s_state_topic[96];
static char s_next_topic[96];
static uint8_t s_state_payload[MQTT_SCHEDULE_PAYLOAD_MAX];
static size_t s_state_payload_len;
static bool s_state_dirty;
static char s_next_payload[128];
static size_t s_next_payload_len;
static bool s_next_dirty;

static void mqtt_schedule_format_topics(void)
{
    const char *device_id = mqtt_client_device_id();

    s_state_topic[0] = '\0';
    s_next_topic[0] = '\0';

    if (device_id == NULL || device_id[0] == '\0') {
        return;
    }

    if (mqtt_topic_format(s_state_topic, sizeof(s_state_topic), device_id, "schedule/state")
            != PORT_OK) {
        s_state_topic[0] = '\0';
    }

    if (mqtt_topic_format(s_next_topic, sizeof(s_next_topic), device_id, "schedule/next")
            != PORT_OK) {
        s_next_topic[0] = '\0';
    }
}

static void mqtt_schedule_stage_payloads(void)
{
    int written;

    written = schedule_format_state_json((char *)s_state_payload, sizeof(s_state_payload));
    if (written > 0) {
        s_state_payload_len = (size_t)written;
        s_state_dirty = true;
    }

    written = schedule_format_next_json(s_next_payload, sizeof(s_next_payload));
    if (written > 0) {
        s_next_payload_len = (size_t)written;
        s_next_dirty = true;
    } else {
        s_next_payload[0] = '\0';
        s_next_payload_len = 0;
        s_next_dirty = true;
    }
}

void mqtt_schedule_request_publish(void)
{
    mqtt_schedule_format_topics();
    mqtt_schedule_stage_payloads();
}

void mqtt_schedule_connect_snapshot(void)
{
    mqtt_schedule_request_publish();
}

bool mqtt_schedule_drain(const mqtt_port_t *mqtt)
{
    bool published = false;

    if (mqtt == NULL || mqtt->publish == NULL || mqtt->is_connected == NULL ||
        !mqtt->is_connected()) {
        return false;
    }

    if (s_state_topic[0] == '\0') {
        mqtt_schedule_format_topics();
    }

    if (s_state_dirty && s_state_topic[0] != '\0' && s_state_payload_len > 0u) {
        if (mqtt->publish(s_state_topic,
                          s_state_payload,
                          s_state_payload_len,
                          1,
                          true)
                == PORT_OK) {
            s_state_dirty = false;
            published = true;
        }
    }

    if (s_next_dirty && s_next_topic[0] != '\0') {
        if (mqtt->publish(s_next_topic,
                          s_next_payload,
                          s_next_payload_len,
                          1,
                          true)
                == PORT_OK) {
            s_next_dirty = false;
            published = true;
        }
    }

    return published;
}

static bool mqtt_schedule_json_has_legacy_days(const char *json, size_t len)
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

static bool mqtt_schedule_json_find_repeat_days(const char *json,
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
        int matched;

        while (cursor < end && (*cursor == ' ' || *cursor == '\t')) {
            cursor++;
        }

        if (cursor < end && *cursor == ']') {
            break;
        }

        matched = sscanf(cursor, "%u", &value);
        if (matched != 1 || value > 6u) {
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

static bool mqtt_schedule_json_find_uint(const char *json,
                                         size_t len,
                                         const char *key,
                                         unsigned *out)
{
    char pattern[32];
    const char *cursor;
    unsigned value = 0;

    if (json == NULL || key == NULL || out == NULL || len == 0) {
        return false;
    }

    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    cursor = strstr(json, pattern);
    if (cursor == NULL || cursor >= json + len) {
        return false;
    }

    cursor += strlen(pattern);
    while (cursor < json + len && (*cursor == ' ' || *cursor == '\t')) {
        cursor++;
    }

    if (sscanf(cursor, "%u", &value) != 1) {
        return false;
    }

    *out = value;
    return true;
}

static bool mqtt_schedule_json_find_bool(const char *json,
                                         size_t len,
                                         const char *key,
                                         bool *out)
{
    char pattern[32];
    const char *cursor;

    if (json == NULL || key == NULL || out == NULL || len == 0) {
        return false;
    }

    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    cursor = strstr(json, pattern);
    if (cursor == NULL || cursor >= json + len) {
        return false;
    }

    cursor += strlen(pattern);
    while (cursor < json + len && (*cursor == ' ' || *cursor == '\t')) {
        cursor++;
    }

    if (strncmp(cursor, "true", 4) == 0) {
        *out = true;
        return true;
    }

    if (strncmp(cursor, "false", 5) == 0) {
        *out = false;
        return true;
    }

    return false;
}

bool mqtt_schedule_handle(mqtt_route_kind_t route,
                          const void *payload,
                          size_t len)
{
    const char *json = payload;
    schedule_slot_config_t cfg;
    unsigned hour = 0;
    unsigned min = 0;
    unsigned g = 0;
    bool enabled = true;
    bool skip = true;
    bool ok = false;

    if (json == NULL || len == 0) {
        return false;
    }

    switch (route) {
    case MQTT_ROUTE_CMD_SCHEDULE_SET:
        if (mqtt_schedule_json_has_legacy_days(json, len)) {
            app_log_info("mqtt", "schedule set rejected");
            return false;
        }

        if (!mqtt_schedule_json_find_uint(json, len, "hour", &hour) ||
            !mqtt_schedule_json_find_uint(json, len, "min", &min) ||
            !mqtt_schedule_json_find_repeat_days(json, len, &cfg.days) ||
            !mqtt_schedule_json_find_uint(json, len, "g", &g)) {
            app_log_info("mqtt", "schedule set rejected");
            return false;
        }

        if (!mqtt_schedule_json_find_bool(json, len, "enabled", &enabled)) {
            enabled = true;
        }

        cfg.hour = (uint8_t)hour;
        cfg.min = (uint8_t)min;
        cfg.g = (uint8_t)g;
        cfg.enabled = enabled;
        ok = schedule_set_slot(&cfg);
        break;

    case MQTT_ROUTE_CMD_SCHEDULE_DELETE:
        if (!mqtt_schedule_json_find_uint(json, len, "hour", &hour) ||
            !mqtt_schedule_json_find_uint(json, len, "min", &min)) {
            app_log_info("mqtt", "schedule delete rejected");
            return false;
        }

        ok = schedule_delete_slot((uint8_t)hour, (uint8_t)min);
        break;

    case MQTT_ROUTE_CMD_SCHEDULE_TOGGLE:
        if (!mqtt_schedule_json_find_uint(json, len, "hour", &hour) ||
            !mqtt_schedule_json_find_uint(json, len, "min", &min)) {
            app_log_info("mqtt", "schedule toggle rejected");
            return false;
        }

        ok = schedule_toggle_slot((uint8_t)hour, (uint8_t)min);
        break;

    case MQTT_ROUTE_CMD_SCHEDULE_SKIP:
        if (!mqtt_schedule_json_find_uint(json, len, "hour", &hour) ||
            !mqtt_schedule_json_find_uint(json, len, "min", &min) ||
            !mqtt_schedule_json_find_bool(json, len, "skip", &skip)) {
            app_log_info("mqtt", "schedule skip rejected");
            return false;
        }

        ok = schedule_skip_slot((uint8_t)hour, (uint8_t)min, skip);
        break;

    case MQTT_ROUTE_CMD_SCHEDULE_ENABLE:
        if (!mqtt_schedule_json_find_bool(json, len, "enabled", &enabled)) {
            app_log_info("mqtt", "schedule enable rejected");
            return false;
        }

        if (schedule_global_enabled() == enabled) {
            return true;
        }

        ok = schedule_set_global_enabled(enabled);
        break;

    case MQTT_ROUTE_CMD_SCHEDULE_TODAY:
        if (!mqtt_schedule_json_find_bool(json, len, "enabled", &enabled)) {
            app_log_info("mqtt", "schedule today rejected");
            return false;
        }

        if (schedule_today_enabled() == enabled) {
            return true;
        }

        ok = schedule_set_today_enabled(enabled);
        break;

    default:
        return false;
    }

    if (!ok) {
        app_log_info("mqtt", "schedule cmd noop route=%d", (int)route);
        return false;
    }

    mqtt_schedule_request_publish();
    return true;
}

void mqtt_schedule_test_reset(void)
{
    s_state_topic[0] = '\0';
    s_next_topic[0] = '\0';
    s_state_payload_len = 0;
    s_state_dirty = false;
    s_next_payload[0] = '\0';
    s_next_payload_len = 0;
    s_next_dirty = false;
}
