/*
 * Hopper level MQTT publisher — spec/30-processes/mqtt-protocol.md § hopper
 */

#include <string.h>

#include "app_log.h"
#include "mqtt_hopper.h"
#include "mqtt_outbox.h"
#include "mqtt_topics.h"
#include "port_err.h"

static char s_hopper_topic[96];
static bool s_level_known;
static hopper_level_state_t s_last_level;
static bool s_last_published_valid;
static hopper_level_state_t s_last_published_level;

static const char *mqtt_hopper_level_string(hopper_level_state_t level)
{
    switch (level) {
    case HOPPER_LEVEL_STATE_NORMAL:
        return "normal";
    case HOPPER_LEVEL_STATE_LOW:
        return "low";
    case HOPPER_LEVEL_STATE_EMPTY:
        return "empty";
    default:
        return "normal";
    }
}

static bool mqtt_hopper_publish(hopper_level_state_t level)
{
    const char *level_str = mqtt_hopper_level_string(level);
    size_t payload_len = strlen(level_str);

    if (s_hopper_topic[0] == '\0') {
        return false;
    }

    if (!mqtt_outbox_enqueue(s_hopper_topic,
                             level_str,
                             payload_len,
                             1,
                             true)) {
        app_log_debug("mqtt", "hopper enqueue failed topic=%s", s_hopper_topic);
        return false;
    }

    s_last_published_valid = true;
    s_last_published_level = level;
    app_log_info("app", "hopper %s", level_str);
    return true;
}

void mqtt_hopper_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_hopper_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_hopper_topic, sizeof(s_hopper_topic), device_id, "hopper")
            != PORT_OK) {
        s_hopper_topic[0] = '\0';
    }
}

void mqtt_hopper_sync(hopper_level_state_t level)
{
    s_last_level = level;
    s_level_known = true;

    if (s_hopper_topic[0] == '\0') {
        return;
    }

    if (s_last_published_valid && s_last_published_level == level) {
        return;
    }

    (void)mqtt_hopper_publish(level);
}

void mqtt_hopper_connect_snapshot(hopper_level_state_t level)
{
    s_last_level = level;
    s_level_known = true;

    if (s_hopper_topic[0] == '\0') {
        return;
    }

    s_last_published_valid = false;
    (void)mqtt_hopper_publish(level);
}

void mqtt_hopper_on_mqtt_connected(void)
{
    if (!s_level_known || s_hopper_topic[0] == '\0') {
        return;
    }

    s_last_published_valid = false;
    (void)mqtt_hopper_publish(s_last_level);
}

void mqtt_hopper_on_outbox_reset(void)
{
    s_last_published_valid = false;
}

void mqtt_hopper_test_reset(void)
{
    s_hopper_topic[0] = '\0';
    s_level_known = false;
    s_last_level = HOPPER_LEVEL_STATE_NORMAL;
    s_last_published_valid = false;
    s_last_published_level = HOPPER_LEVEL_STATE_NORMAL;
}
