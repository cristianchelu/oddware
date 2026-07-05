/*
 * Minimal JSON field helpers for MQTT payloads.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mqtt_json.h"

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

static bool mqtt_json_locate_value(const char *json,
                                    size_t len,
                                    const char *key,
                                    const char **value_out)
{
    char pattern[32];
    const char *cursor;
    const char *end;

    if (json == NULL || key == NULL || value_out == NULL || len == 0u) {
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
        if (cursor >= end) {
            return false;
        }

        *value_out = cursor;
        return true;
    }

    return false;
}

bool mqtt_json_has_key(const char *json, size_t len, const char *key)
{
    const char *value = NULL;

    return mqtt_json_locate_value(json, len, key, &value);
}

bool mqtt_json_find_string(const char *json,
                            size_t len,
                            const char *key,
                            char *out,
                            size_t out_len)
{
    const char *cursor;
    const char *end;

    if (out == NULL || out_len == 0u) {
        return false;
    }

    if (!mqtt_json_locate_value(json, len, key, &cursor)) {
        return false;
    }

    end = json + len;
    if (cursor >= end || *cursor != '"') {
        return false;
    }

    return mqtt_json_copy_string(cursor, end, out, out_len);
}

bool mqtt_json_find_uint(const char *json,
                          size_t len,
                          const char *key,
                          unsigned *out)
{
    const char *cursor;
    const char *end;
    char *endp;
    unsigned long value;

    if (out == NULL) {
        return false;
    }

    if (!mqtt_json_locate_value(json, len, key, &cursor)) {
        return false;
    }

    end = json + len;
    value = strtoul(cursor, &endp, 10);
    if (endp == cursor) {
        return false;
    }

    while (endp < end && (*endp == ' ' || *endp == '\t' ||
                          *endp == '\r' || *endp == '\n')) {
        endp++;
    }
    if (endp < end && *endp != ',' && *endp != '}') {
        return false;
    }

    *out = (unsigned)value;
    return true;
}

bool mqtt_json_find_bool(const char *json,
                          size_t len,
                          const char *key,
                          bool *out)
{
    const char *cursor;

    if (out == NULL) {
        return false;
    }

    if (!mqtt_json_locate_value(json, len, key, &cursor)) {
        return false;
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
