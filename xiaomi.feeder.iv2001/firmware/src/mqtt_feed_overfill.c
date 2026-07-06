/*
 * Overfill protection MQTT publisher — spec/30-processes/mqtt-protocol.md § Overfill protection
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "feed_config.h"
#include "mqtt_feed_overfill.h"
#include "mqtt_json.h"
#include "mqtt_outbox.h"
#include "mqtt_topics.h"
#include "port_err.h"

static char s_feed_overfill_topic[96];
static char s_last_payload[64];
static bool s_last_payload_valid;

bool mqtt_feed_overfill_format_snapshot(char *buf, size_t len)
{
    int written;

    if (buf == NULL || len == 0) {
        return false;
    }

    written = snprintf(buf,
                       len,
                       "{\"enabled\":%s,\"threshold_g\":%u}",
                       feed_config_overfill_enabled_get() ? "true" : "false",
                       (unsigned)feed_config_overfill_threshold_g_get());
    if (written <= 0 || (size_t)written >= len) {
        return false;
    }

    return true;
}

static bool mqtt_feed_overfill_publish(void)
{
    char payload[64];

    if (s_feed_overfill_topic[0] == '\0') {
        return false;
    }

    if (!mqtt_feed_overfill_format_snapshot(payload, sizeof(payload))) {
        return false;
    }

    if (s_last_payload_valid && strcmp(s_last_payload, payload) == 0) {
        return true;
    }

    if (!mqtt_outbox_enqueue(s_feed_overfill_topic,
                             payload,
                             strlen(payload),
                             1,
                             true)) {
        app_log_debug("mqtt", "feed overfill enqueue failed topic=%s", s_feed_overfill_topic);
        return false;
    }

    (void)snprintf(s_last_payload, sizeof(s_last_payload), "%s", payload);
    s_last_payload_valid = true;
    return true;
}

static port_err_t mqtt_feed_overfill_apply(bool has_enabled,
                                           bool enabled,
                                           bool has_threshold,
                                           uint8_t threshold_g)
{
    bool changed = false;

    if (!has_enabled && !has_threshold) {
        return PORT_ERR_INVALID_ARG;
    }

    if (has_enabled) {
        if (feed_config_overfill_enabled_get() != enabled) {
            if (!feed_config_overfill_enabled_set(enabled)) {
                return PORT_ERR_IO;
            }

            changed = true;
        }
    }

    if (has_threshold) {
        if (threshold_g < FEED_OVERFILL_THRESHOLD_G_MIN ||
            threshold_g > FEED_OVERFILL_THRESHOLD_G_MAX) {
            return PORT_ERR_INVALID_ARG;
        }

        if (feed_config_overfill_threshold_g_get() != threshold_g) {
            if (!feed_config_overfill_threshold_g_set(threshold_g)) {
                return PORT_ERR_IO;
            }

            changed = true;
        }
    }

    if (changed) {
        mqtt_feed_overfill_publish_snapshot();
    }

    return PORT_OK;
}

void mqtt_feed_overfill_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_feed_overfill_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_feed_overfill_topic,
                          sizeof(s_feed_overfill_topic),
                          device_id,
                          "feed/overfill")
            != PORT_OK) {
        s_feed_overfill_topic[0] = '\0';
    }
}

void mqtt_feed_overfill_publish_snapshot(void)
{
    (void)mqtt_feed_overfill_publish();
}

void mqtt_feed_overfill_connect_snapshot(void)
{
    s_last_payload_valid = false;
    mqtt_feed_overfill_publish_snapshot();
}

port_err_t mqtt_feed_overfill_handle(const void *payload, size_t len)
{
    bool enabled = false;
    bool has_enabled = false;
    bool has_threshold = false;
    unsigned threshold_g = 0u;

    if (payload == NULL || len == 0u) {
        return PORT_ERR_INVALID_ARG;
    }

    if (mqtt_json_find_bool((const char *)payload, len, "enabled", &enabled)) {
        has_enabled = true;
    } else if (mqtt_json_has_key((const char *)payload, len, "enabled")) {
        return PORT_ERR_INVALID_ARG;
    }

    if (mqtt_json_find_uint((const char *)payload, len, "threshold_g", &threshold_g)) {
        has_threshold = true;
    } else if (mqtt_json_has_key((const char *)payload, len, "threshold_g")) {
        return PORT_ERR_INVALID_ARG;
    }

    return mqtt_feed_overfill_apply(has_enabled,
                                    enabled,
                                    has_threshold,
                                    (uint8_t)threshold_g);
}

void mqtt_feed_overfill_test_reset(void)
{
    s_feed_overfill_topic[0] = '\0';
    s_last_payload[0] = '\0';
    s_last_payload_valid = false;
}
