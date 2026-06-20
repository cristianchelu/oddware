/*
 * Retained config MQTT publisher — spec/30-processes/mqtt-protocol.md § Config snapshot
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "config_port.h"
#include "mqtt_config.h"
#include "mqtt_outbox.h"
#include "mqtt_topics.h"
#include "time_config.h"
#include "time_sync.h"
#include "tz_rule.h"

static char s_config_topic[96];
static char s_last_payload[256];
static bool s_last_payload_valid;

static const char *mqtt_json_skip_string(const char *cursor, const char *end)
{
    if (cursor >= end || *cursor != '"') {
        return NULL;
    }

    cursor++;
    while (cursor < end) {
        if (*cursor == '"') {
            return cursor + 1;
        }
        if (*cursor == '\\') {
            cursor++;
            if (cursor >= end) {
                return NULL;
            }
        }
        cursor++;
    }

    return NULL;
}

static bool mqtt_json_copy_string(const char *cursor,
                                  const char *end,
                                  char *out,
                                  size_t out_len)
{
    size_t out_i = 0;

    if (cursor >= end || *cursor != '"') {
        return false;
    }

    cursor++;
    while (cursor < end) {
        char c = *cursor++;

        if (c == '"') {
            if (out_i >= out_len) {
                return false;
            }
            out[out_i] = '\0';
            return true;
        }

        if (c == '\\') {
            if (cursor >= end) {
                return false;
            }
            c = *cursor++;
        }

        if (out_i + 1 >= out_len) {
            return false;
        }

        out[out_i++] = c;
    }

    return false;
}

static bool mqtt_json_find_string(const char *json,
                                  size_t len,
                                  const char *key,
                                  char *out,
                                  size_t out_len)
{
    char pattern[32];
    const char *cursor;
    const char *end;

    if (json == NULL || key == NULL || out == NULL || out_len == 0) {
        return false;
    }

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    end = json + len;
    cursor = json;

    while (cursor < end) {
        const char *found = strstr(cursor, pattern);

        if (found == NULL || found >= end) {
            return false;
        }

        cursor = found + strlen(pattern);
        while (cursor < end && (*cursor == ' ' || *cursor == '\t' ||
                                *cursor == '\r' || *cursor == '\n')) {
            cursor++;
        }
        if (cursor >= end || *cursor != ':') {
            continue;
        }
        cursor++;
        while (cursor < end && (*cursor == ' ' || *cursor == '\t' ||
                                *cursor == '\r' || *cursor == '\n')) {
            cursor++;
        }
        if (cursor >= end || *cursor != '"') {
            return false;
        }

        return mqtt_json_copy_string(cursor, end, out, out_len);
    }

    return false;
}

static bool mqtt_json_has_unknown_keys(const char *json, size_t len)
{
    const char *cursor = json;
    const char *end = json + len;

    if (cursor == NULL || len == 0) {
        return true;
    }

    while (cursor < end && *cursor != '{') {
        cursor++;
    }
    if (cursor >= end) {
        return true;
    }
    cursor++;

    while (cursor < end) {
        char key[32];
        size_t key_len;
        const char *key_start;
        const char *key_end;
        const char *after_value;

        while (cursor < end &&
               (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' ||
                *cursor == '\n' || *cursor == ',')) {
            cursor++;
        }

        if (cursor >= end || *cursor == '}') {
            break;
        }

        if (*cursor != '"') {
            return true;
        }

        key_start = cursor + 1;
        key_end = strchr(key_start, '"');
        if (key_end == NULL || key_end >= end) {
            return true;
        }

        key_len = (size_t)(key_end - key_start);
        if (key_len >= sizeof(key)) {
            return true;
        }

        memcpy(key, key_start, key_len);
        key[key_len] = '\0';

        if (strcmp(key, "tz_rule") != 0 && strcmp(key, "tz_label") != 0) {
            return true;
        }

        cursor = key_end + 1;
        while (cursor < end && *cursor != ':') {
            cursor++;
        }
        if (cursor >= end) {
            return true;
        }
        cursor++;

        while (cursor < end &&
               (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' ||
                *cursor == '\n')) {
            cursor++;
        }

        if (cursor >= end) {
            return true;
        }

        if (*cursor == '"') {
            after_value = mqtt_json_skip_string(cursor, end);
            if (after_value == NULL) {
                return true;
            }
            cursor = after_value;
        } else {
            while (cursor < end && *cursor != ',' && *cursor != '}') {
                cursor++;
            }
        }
    }

    return false;
}

static void mqtt_json_escape_string(const char *in, char *out, size_t out_len)
{
    size_t i = 0;
    size_t j = 0;

    if (in == NULL || out == NULL || out_len == 0) {
        return;
    }

    out[j++] = '"';
    for (i = 0; in[i] != '\0' && j + 2 < out_len; i++) {
        if (in[i] == '"' || in[i] == '\\') {
            if (j + 2 >= out_len) {
                break;
            }
            out[j++] = '\\';
        }
        out[j++] = in[i];
    }
    out[j++] = '"';
    out[j] = '\0';
}

bool mqtt_config_format_snapshot(char *buf, size_t len)
{
    char posix[TZ_RULE_POSIX_MAX];
    char label[TZ_RULE_LABEL_MAX];
    char label_json[TZ_RULE_LABEL_MAX + 8];
    int64_t utc_epoch = 0;
    bool synced = time_sync_is_valid();
    int written;

    if (buf == NULL || len == 0) {
        return false;
    }

    if (tz_rule_load_posix(config_port_get(), posix, sizeof(posix)) != PORT_OK) {
        strcpy(posix, "UTC0");
    }

    if (tz_rule_label_load(config_port_get(), label, sizeof(label)) != PORT_OK) {
        label[0] = '\0';
    }

    mqtt_json_escape_string(label, label_json, sizeof(label_json));

    if (synced) {
        (void)time_sync_get_utc_epoch(&utc_epoch);
    }

    written = snprintf(buf,
                         len,
                         "{\"tz_rule\":\"%s\",\"tz_label\":%s,\"time_synced\":%s,\"utc_epoch\":%lu}",
                         posix,
                         label_json,
                         synced ? "true" : "false",
                         (unsigned long)(synced ? utc_epoch : 0));
    return written > 0 && (size_t)written < len;
}

static bool mqtt_config_publish_payload(const char *payload, size_t payload_len)
{
    if (s_config_topic[0] == '\0') {
        return false;
    }

    if (s_last_payload_valid &&
        strlen(s_last_payload) == payload_len &&
        memcmp(s_last_payload, payload, payload_len) == 0) {
        return true;
    }

    if (!mqtt_outbox_enqueue(s_config_topic, payload, payload_len, 1, true)) {
        app_log_debug("mqtt", "config enqueue failed topic=%s", s_config_topic);
        return false;
    }

    if (payload_len + 1 <= sizeof(s_last_payload)) {
        memcpy(s_last_payload, payload, payload_len);
        s_last_payload[payload_len] = '\0';
        s_last_payload_valid = true;
    }

    return true;
}

void mqtt_config_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_config_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_config_topic, sizeof(s_config_topic), device_id, "config")
            != PORT_OK) {
        s_config_topic[0] = '\0';
    }
}

void mqtt_config_publish_snapshot(void)
{
    char payload[256];

    if (!mqtt_config_format_snapshot(payload, sizeof(payload))) {
        return;
    }

    (void)mqtt_config_publish_payload(payload, strlen(payload));
}

void mqtt_config_connect_snapshot(void)
{
    s_last_payload_valid = false;
    mqtt_config_publish_snapshot();
}

port_err_t mqtt_config_handle(const void *payload, size_t len)
{
    const char *json = payload;
    char tz_posix[TZ_RULE_POSIX_MAX];
    char tz_label[TZ_RULE_LABEL_MAX];
    time_config_patch_t patch;
    bool have_rule = false;
    bool have_label = false;

    if (json == NULL || len == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    if (mqtt_json_has_unknown_keys(json, len)) {
        return PORT_ERR_INVALID_ARG;
    }

    have_rule = mqtt_json_find_string(json, len, "tz_rule", tz_posix, sizeof(tz_posix));
    have_label = mqtt_json_find_string(json, len, "tz_label", tz_label, sizeof(tz_label));

    if (!have_rule && !have_label) {
        return PORT_ERR_INVALID_ARG;
    }

    patch.tz_rule_posix = have_rule ? tz_posix : NULL;
    patch.tz_label = have_label ? tz_label : NULL;
    return time_config_apply(config_port_get(), &patch);
}

void mqtt_config_test_reset(void)
{
    s_config_topic[0] = '\0';
    s_last_payload[0] = '\0';
    s_last_payload_valid = false;
}
