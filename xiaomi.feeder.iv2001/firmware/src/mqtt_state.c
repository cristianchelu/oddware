/*
 * Device condition MQTT publisher — spec/30-processes/mqtt-protocol.md § Device condition
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "mqtt_outbox.h"
#include "mqtt_state.h"
#include "mqtt_topics.h"
#include "port_err.h"

static char s_device_id[32];
static char s_state_topic[96];
static bool s_has_bowl_error;
static bool s_bowl_error_known;
static bool s_last_published_valid;
static bool s_last_published_bowl_error;
static bool s_publish_pending;
static bool s_pending_bowl_error;
static bool s_drained_fn_registered;

static void mqtt_state_outbox_drained(const char *topic,
                                      const void *payload,
                                      size_t len,
                                      void *ctx)
{
    (void)payload;
    (void)len;
    (void)ctx;

    if (!s_publish_pending || topic == NULL || s_state_topic[0] == '\0') {
        return;
    }

    if (strcmp(topic, s_state_topic) != 0) {
        return;
    }

    s_last_published_valid = true;
    s_last_published_bowl_error = s_pending_bowl_error;
    s_publish_pending = false;
}

static void mqtt_state_register_drained_fn(void)
{
    if (s_drained_fn_registered) {
        return;
    }

    mqtt_outbox_set_drained_fn(mqtt_state_outbox_drained, NULL);
    s_drained_fn_registered = true;
}

static bool mqtt_state_publish(bool bowl_error)
{
    char payload[32];
    int written;

    if (s_state_topic[0] == '\0') {
        return false;
    }

    written = snprintf(payload,
                       sizeof(payload),
                       "{\"bowl_error\": %s}",
                       bowl_error ? "true" : "false");
    if (written <= 0 || (size_t)written >= sizeof(payload)) {
        return false;
    }

    if (!mqtt_outbox_enqueue(s_state_topic, payload, (size_t)written, 1, true)) {
        app_log_debug("mqtt", "state enqueue failed topic=%s", s_state_topic);
        return false;
    }

    s_publish_pending = true;
    s_pending_bowl_error = bowl_error;
    return true;
}

void mqtt_state_set_device_id(const char *device_id)
{
    mqtt_state_register_drained_fn();

    if (device_id == NULL || device_id[0] == '\0') {
        s_device_id[0] = '\0';
        s_state_topic[0] = '\0';
        return;
    }

    strncpy(s_device_id, device_id, sizeof(s_device_id) - 1);
    s_device_id[sizeof(s_device_id) - 1] = '\0';

    if (mqtt_topic_format(s_state_topic, sizeof(s_state_topic), s_device_id, "state")
            != PORT_OK) {
        s_state_topic[0] = '\0';
    }
}

void mqtt_state_sync(bool bowl_error)
{
    s_has_bowl_error = bowl_error;
    s_bowl_error_known = true;

    if (s_state_topic[0] == '\0') {
        return;
    }

    if (!s_publish_pending && s_last_published_valid &&
        s_last_published_bowl_error == bowl_error) {
        return;
    }

    if (s_publish_pending) {
        return;
    }

    (void)mqtt_state_publish(bowl_error);
}

void mqtt_state_on_mqtt_connected(void)
{
    if (!s_bowl_error_known || s_state_topic[0] == '\0') {
        return;
    }

    s_last_published_valid = false;
    s_publish_pending = false;
    (void)mqtt_state_publish(s_has_bowl_error);
}

void mqtt_state_on_outbox_reset(void)
{
    s_publish_pending = false;
    s_last_published_valid = false;
}

void mqtt_state_test_reset(void)
{
    s_device_id[0] = '\0';
    s_state_topic[0] = '\0';
    s_has_bowl_error = false;
    s_bowl_error_known = false;
    s_last_published_valid = false;
    s_last_published_bowl_error = false;
    s_publish_pending = false;
    s_pending_bowl_error = false;
}
