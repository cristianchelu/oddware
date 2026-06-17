/*
 * Bowl weight MQTT publisher — spec/30-processes/mqtt-protocol.md § Bowl weight
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "app_log.h"
#include "mqtt_bowl_weight.h"
#include "mqtt_outbox.h"
#include "mqtt_topics.h"
#include "port_err.h"

static char s_bowl_weight_topic[96];
static bowl_grams_status_t s_last_status;
static int32_t s_last_grams;
static bool s_value_known;
static bool s_last_published_valid;
static bool s_last_published_known;
static int32_t s_last_published_grams;
static uint32_t s_last_publish_ms;
static bool s_coalesce_pending;
static bool s_coalesce_known;
static int32_t s_coalesce_grams;

static uint32_t mqtt_bowl_weight_now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * (TickType_t)portTICK_PERIOD_MS);
}

static void mqtt_bowl_weight_mark_published(bool known, int32_t grams)
{
    s_last_published_valid = true;
    s_last_published_known = known;
    s_last_published_grams = grams;
    s_last_publish_ms = mqtt_bowl_weight_now_ms();
    s_coalesce_pending = false;
}

static bool mqtt_bowl_weight_publish(bool known, int32_t grams)
{
    char payload[16];
    const void *pub_ptr;
    size_t payload_len;
    int written;

    if (s_bowl_weight_topic[0] == '\0') {
        return false;
    }

    if (known) {
        written = snprintf(payload, sizeof(payload), "%ld", (long)grams);
        if (written <= 0 || (size_t)written >= sizeof(payload)) {
            return false;
        }
        pub_ptr = payload;
        payload_len = (size_t)written;
        app_log_info("app", "bowl g %ld", (long)grams);
    } else {
        pub_ptr = "";
        payload_len = 0u;
        app_log_info("app", "bowl g unknown");
    }

    if (!mqtt_outbox_enqueue(s_bowl_weight_topic, pub_ptr, payload_len, 1, true)) {
        app_log_debug("mqtt", "bowl_weight enqueue failed topic=%s", s_bowl_weight_topic);
        return false;
    }

    mqtt_bowl_weight_mark_published(known, grams);
    return true;
}

static bool mqtt_bowl_weight_should_publish(bowl_grams_status_t status,
                                            int32_t grams,
                                            bool force)
{
    bool known = (status == BOWL_GRAMS_KNOWN);

    if (force) {
        return true;
    }

    if (!s_last_published_valid) {
        return true;
    }

    if (known != s_last_published_known) {
        return true;
    }

    if (!known) {
        return false;
    }

    if (grams < s_last_published_grams) {
        return (s_last_published_grams - grams) >= MQTT_BOWL_WEIGHT_CHANGE_THRESHOLD_G;
    }

    return (grams - s_last_published_grams) >= MQTT_BOWL_WEIGHT_CHANGE_THRESHOLD_G;
}

static void mqtt_bowl_weight_flush_coalesce(bool force)
{
    uint32_t now_ms = mqtt_bowl_weight_now_ms();

    if (!s_coalesce_pending) {
        return;
    }

    if (!force &&
        (now_ms - s_last_publish_ms) < MQTT_BOWL_WEIGHT_COALESCE_MS) {
        return;
    }

    (void)mqtt_bowl_weight_publish(s_coalesce_known, s_coalesce_grams);
}

static void mqtt_bowl_weight_enqueue(bool known, int32_t grams, bool force)
{
    uint32_t now_ms = mqtt_bowl_weight_now_ms();

    if (!force && s_last_published_valid &&
        known == s_last_published_known &&
        (now_ms - s_last_publish_ms) < MQTT_BOWL_WEIGHT_COALESCE_MS) {
        s_coalesce_pending = true;
        s_coalesce_known = known;
        s_coalesce_grams = grams;
        return;
    }

    (void)mqtt_bowl_weight_publish(known, grams);
}

void mqtt_bowl_weight_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_bowl_weight_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_bowl_weight_topic,
                          sizeof(s_bowl_weight_topic),
                          device_id,
                          "bowl_weight")
            != PORT_OK) {
        s_bowl_weight_topic[0] = '\0';
    }
}

void mqtt_bowl_weight_sync(bowl_grams_status_t status, int32_t grams, bool force)
{
    s_last_status = status;
    s_last_grams = grams;
    s_value_known = true;

    if (s_bowl_weight_topic[0] == '\0') {
        return;
    }

    mqtt_bowl_weight_flush_coalesce(force);

    if (!mqtt_bowl_weight_should_publish(status, grams, force)) {
        return;
    }

    mqtt_bowl_weight_enqueue(status == BOWL_GRAMS_KNOWN, grams, force);
}

void mqtt_bowl_weight_on_mqtt_connected(void)
{
    if (!s_value_known || s_bowl_weight_topic[0] == '\0') {
        return;
    }

    s_last_published_valid = false;
    s_coalesce_pending = false;
    mqtt_bowl_weight_enqueue(s_last_status == BOWL_GRAMS_KNOWN, s_last_grams, true);
}

void mqtt_bowl_weight_on_outbox_reset(void)
{
    s_last_published_valid = false;
    s_coalesce_pending = false;
}

void mqtt_bowl_weight_test_reset(void)
{
    s_bowl_weight_topic[0] = '\0';
    s_last_status = BOWL_GRAMS_UNKNOWN;
    s_last_grams = 0;
    s_value_known = false;
    s_last_published_valid = false;
    s_last_published_known = false;
    s_last_published_grams = 0;
    s_last_publish_ms = 0u;
    s_coalesce_pending = false;
    s_coalesce_known = false;
    s_coalesce_grams = 0;
}
