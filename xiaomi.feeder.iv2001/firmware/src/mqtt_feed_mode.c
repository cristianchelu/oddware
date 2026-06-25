/*
 * Feed mode MQTT publisher — spec/30-processes/mqtt-protocol.md § Feed mode
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "feed_config.h"
#include "mqtt_feed_mode.h"
#include "mqtt_outbox.h"
#include "mqtt_topics.h"
#include "port_err.h"

static char s_feed_mode_topic[96];
static char s_last_payload[16];
static bool s_last_payload_valid;

static bool mqtt_feed_mode_publish(dispense_mode_t mode)
{
    const char *payload = feed_config_mode_string(mode);
    size_t payload_len = strlen(payload);

    if (s_feed_mode_topic[0] == '\0') {
        return false;
    }

    if (s_last_payload_valid &&
        strcmp(s_last_payload, payload) == 0) {
        return true;
    }

    if (!mqtt_outbox_enqueue(s_feed_mode_topic, payload, payload_len, 1, true)) {
        app_log_debug("mqtt", "feed mode enqueue failed topic=%s", s_feed_mode_topic);
        return false;
    }

    (void)snprintf(s_last_payload, sizeof(s_last_payload), "%s", payload);
    s_last_payload_valid = true;
    return true;
}

static port_err_t mqtt_feed_mode_parse_payload(const void *payload,
                                                size_t len,
                                                dispense_mode_t *mode_out)
{
    char buf[17];
    size_t start = 0u;
    size_t end;

    if (payload == NULL || mode_out == NULL || len == 0u) {
        return PORT_ERR_INVALID_ARG;
    }

    if (len >= sizeof(buf)) {
        return PORT_ERR_INVALID_ARG;
    }

    memcpy(buf, payload, len);
    buf[len] = '\0';

    while (buf[start] != '\0' &&
           isspace((unsigned char)buf[start])) {
        start++;
    }

    end = strlen(buf + start);
    while (end > 0u && isspace((unsigned char)buf[start + end - 1u])) {
        buf[start + end - 1u] = '\0';
        end--;
    }

    if (strcmp(buf + start, "compensated") == 0) {
        *mode_out = DISPENSE_MODE_COMPENSATED;
        return PORT_OK;
    }

    if (strcmp(buf + start, "open_loop") == 0) {
        *mode_out = DISPENSE_MODE_OPEN_LOOP;
        return PORT_OK;
    }

    return PORT_ERR_INVALID_ARG;
}

void mqtt_feed_mode_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_feed_mode_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_feed_mode_topic,
                          sizeof(s_feed_mode_topic),
                          device_id,
                          "feed/mode")
            != PORT_OK) {
        s_feed_mode_topic[0] = '\0';
    }
}

void mqtt_feed_mode_publish_snapshot(void)
{
    (void)mqtt_feed_mode_publish(feed_config_mode_get());
}

void mqtt_feed_mode_connect_snapshot(void)
{
    s_last_payload_valid = false;
    mqtt_feed_mode_publish_snapshot();
}

port_err_t mqtt_feed_mode_apply(dispense_mode_t mode)
{
    if (mode != DISPENSE_MODE_OPEN_LOOP && mode != DISPENSE_MODE_COMPENSATED) {
        return PORT_ERR_INVALID_ARG;
    }

    if (feed_config_mode_get() == mode) {
        return PORT_OK;
    }

    if (!feed_config_mode_set(mode)) {
        return PORT_ERR_IO;
    }

    mqtt_feed_mode_publish_snapshot();
    return PORT_OK;
}

port_err_t mqtt_feed_mode_handle(const void *payload, size_t len)
{
    dispense_mode_t mode;
    port_err_t err;

    err = mqtt_feed_mode_parse_payload(payload, len, &mode);
    if (err != PORT_OK) {
        return err;
    }

    return mqtt_feed_mode_apply(mode);
}

void mqtt_feed_mode_test_reset(void)
{
    s_feed_mode_topic[0] = '\0';
    s_last_payload[0] = '\0';
    s_last_payload_valid = false;
}
