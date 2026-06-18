/*
 * Battery pack voltage MQTT publisher — spec/30-processes/mqtt-protocol.md § Battery pack voltage
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "mqtt_battery_voltage.h"
#include "mqtt_outbox.h"
#include "mqtt_topics.h"
#include "port_err.h"

static char s_voltage_topic[96];
static uint16_t s_last_mv;
static bool s_mv_known;
static bool s_last_published_valid;
static uint16_t s_last_published_mv;

static bool mqtt_battery_voltage_publish(uint16_t pack_mv)
{
    char payload[8];
    int written;

    if (s_voltage_topic[0] == '\0') {
        return false;
    }

    written = snprintf(payload, sizeof(payload), "%u", (unsigned)pack_mv);
    if (written <= 0 || (size_t)written >= sizeof(payload)) {
        return false;
    }

    if (!mqtt_outbox_enqueue(s_voltage_topic, payload, (size_t)written, 1, true)) {
        app_log_debug("mqtt", "battery_voltage enqueue failed topic=%s", s_voltage_topic);
        return false;
    }

    s_last_published_valid = true;
    s_last_published_mv = pack_mv;
    app_log_info("app", "battery %u mV", (unsigned)pack_mv);
    return true;
}

static bool mqtt_battery_voltage_should_publish(uint16_t pack_mv, bool force)
{
    uint16_t delta;

    if (force) {
        return true;
    }

    if (!s_last_published_valid) {
        return true;
    }

    if (pack_mv >= s_last_published_mv) {
        delta = pack_mv - s_last_published_mv;
    } else {
        delta = s_last_published_mv - pack_mv;
    }

    return delta >= MQTT_BATTERY_VOLTAGE_CHANGE_THRESHOLD_MV;
}

void mqtt_battery_voltage_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_voltage_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_voltage_topic,
                          sizeof(s_voltage_topic),
                          device_id,
                          "battery_voltage")
            != PORT_OK) {
        s_voltage_topic[0] = '\0';
    }
}

void mqtt_battery_voltage_sync(uint16_t pack_mv, bool force)
{
    if (s_voltage_topic[0] == '\0') {
        return;
    }

    s_last_mv = pack_mv;
    s_mv_known = true;

    if (!mqtt_battery_voltage_should_publish(pack_mv, force)) {
        return;
    }

    (void)mqtt_battery_voltage_publish(pack_mv);
}

void mqtt_battery_voltage_on_mqtt_connected(void)
{
    if (!s_mv_known || s_voltage_topic[0] == '\0') {
        return;
    }

    s_last_published_valid = false;
    (void)mqtt_battery_voltage_publish(s_last_mv);
}

void mqtt_battery_voltage_on_outbox_reset(void)
{
    s_last_published_valid = false;
}

void mqtt_battery_voltage_test_reset(void)
{
    s_voltage_topic[0] = '\0';
    s_last_mv = 0u;
    s_mv_known = false;
    s_last_published_valid = false;
    s_last_published_mv = 0u;
}
