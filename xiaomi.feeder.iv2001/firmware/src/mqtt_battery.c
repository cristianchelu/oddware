/*
 * Battery MQTT publisher — spec/30-processes/mqtt-protocol.md § Battery
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "mqtt_battery.h"
#include "mqtt_outbox.h"
#include "mqtt_topics.h"
#include "port_err.h"

static char s_battery_topic[96];
static uint8_t s_last_pct;
static bool s_last_known;
static bool s_pct_known;
static bool s_last_published_valid;
static bool s_last_published_known;
static uint8_t s_last_published_pct;

static bool mqtt_battery_publish(bool known, uint8_t pct)
{
    const void *pub_ptr;
    size_t payload_len;
    char payload[8];
    int written;

    if (s_battery_topic[0] == '\0') {
        return false;
    }

    if (known) {
        written = snprintf(payload, sizeof(payload), "%u", (unsigned)pct);
        if (written <= 0 || (size_t)written >= sizeof(payload)) {
            return false;
        }
        pub_ptr = payload;
        payload_len = (size_t)written;
        app_log_info("app", "battery %u%%", (unsigned)pct);
    } else {
        pub_ptr = MQTT_BATTERY_PAYLOAD_UNKNOWN;
        payload_len = sizeof(MQTT_BATTERY_PAYLOAD_UNKNOWN) - 1u;
        app_log_info("app", "battery unknown");
    }

    if (!mqtt_outbox_enqueue(s_battery_topic, pub_ptr, payload_len, 1, true)) {
        app_log_debug("mqtt", "battery enqueue failed topic=%s", s_battery_topic);
        return false;
    }

    s_last_published_valid = true;
    s_last_published_known = known;
    s_last_published_pct = pct;
    return true;
}

static bool mqtt_battery_should_publish(bool known, uint8_t pct, bool force)
{
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

    if (pct >= s_last_published_pct) {
        return (unsigned)(pct - s_last_published_pct) >= MQTT_BATTERY_CHANGE_THRESHOLD_PCT;
    }

    return (unsigned)(s_last_published_pct - pct) >= MQTT_BATTERY_CHANGE_THRESHOLD_PCT;
}

void mqtt_battery_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_battery_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_battery_topic,
                          sizeof(s_battery_topic),
                          device_id,
                          "battery")
            != PORT_OK) {
        s_battery_topic[0] = '\0';
    }
}

void mqtt_battery_sync(bool known, uint8_t pct, bool force)
{
    if (known && pct > 100u) {
        pct = 100u;
    }

    if (s_battery_topic[0] == '\0') {
        return;
    }

    s_last_known = known;
    s_last_pct = pct;
    s_pct_known = true;

    if (!mqtt_battery_should_publish(known, pct, force)) {
        return;
    }

    (void)mqtt_battery_publish(known, pct);
}

void mqtt_battery_on_mqtt_connected(void)
{
    if (!s_pct_known || s_battery_topic[0] == '\0') {
        return;
    }

    s_last_published_valid = false;
    (void)mqtt_battery_publish(s_last_known, s_last_pct);
}

void mqtt_battery_on_outbox_reset(void)
{
    s_last_published_valid = false;
}

void mqtt_battery_test_reset(void)
{
    s_battery_topic[0] = '\0';
    s_last_pct = 0u;
    s_last_known = false;
    s_pct_known = false;
    s_last_published_valid = false;
    s_last_published_known = false;
    s_last_published_pct = 0u;
}
