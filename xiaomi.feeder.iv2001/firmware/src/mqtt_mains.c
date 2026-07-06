/*
 * Mains MQTT publisher — spec/30-processes/mqtt-protocol.md § Mains
 */

#include <string.h>

#include "app_log.h"
#include "mqtt_mains.h"
#include "mqtt_outbox.h"
#include "mqtt_topics.h"
#include "port_err.h"

static char s_mains_topic[96];
static bool s_mains_known;
static bool s_last_mains;
static bool s_last_published_valid;
static bool s_last_published_mains;

static bool mqtt_mains_publish(bool mains_connected)
{
    const char *payload = mains_connected ? "ON" : "OFF";

    if (s_mains_topic[0] == '\0') {
        return false;
    }

    if (!mqtt_outbox_enqueue(s_mains_topic,
                             payload,
                             strlen(payload),
                             1,
                             true)) {
        app_log_debug("mqtt", "mains enqueue failed topic=%s", s_mains_topic);
        return false;
    }

    s_last_published_valid = true;
    s_last_published_mains = mains_connected;
    app_log_info("app", "mains %s", payload);
    return true;
}

void mqtt_mains_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_mains_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_mains_topic, sizeof(s_mains_topic), device_id, "mains")
            != PORT_OK) {
        s_mains_topic[0] = '\0';
    }
}

void mqtt_mains_sync(bool mains_connected)
{
    s_last_mains = mains_connected;
    s_mains_known = true;

    if (s_mains_topic[0] == '\0') {
        return;
    }

    if (s_last_published_valid && s_last_published_mains == mains_connected) {
        return;
    }

    (void)mqtt_mains_publish(mains_connected);
}

void mqtt_mains_connect_snapshot(bool mains_connected)
{
    s_last_mains = mains_connected;
    s_mains_known = true;

    if (s_mains_topic[0] == '\0') {
        return;
    }

    s_last_published_valid = false;
    (void)mqtt_mains_publish(mains_connected);
}

void mqtt_mains_on_mqtt_connected(void)
{
    if (!s_mains_known || s_mains_topic[0] == '\0') {
        return;
    }

    s_last_published_valid = false;
    (void)mqtt_mains_publish(s_last_mains);
}

void mqtt_mains_on_outbox_reset(void)
{
    s_last_published_valid = false;
}

void mqtt_mains_test_reset(void)
{
    s_mains_topic[0] = '\0';
    s_mains_known = false;
    s_last_mains = false;
    s_last_published_valid = false;
    s_last_published_mains = false;
}

bool mqtt_mains_format_wire(char *buf, size_t len)
{
    if (!s_mains_known || buf == NULL || len < 4) {
        return false;
    }

    strncpy(buf, s_last_mains ? "ON" : "OFF", len - 1);
    buf[len - 1] = '\0';
    return true;
}
