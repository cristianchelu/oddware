/*
 * Schedule MQTT publisher — spec/30-processes/mqtt-protocol.md § Schedule
 */

#include "mqtt_schedule.h"

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "mqtt_client.h"
#include "mqtt_topics.h"
#include "port_err.h"
#include "schedule.h"
#include "schedule_cmd.h"

#define MQTT_SCHEDULE_PAYLOAD_MAX 4096u

static char s_state_topic[96];
static char s_next_topic[96];
static uint8_t s_state_payload[MQTT_SCHEDULE_PAYLOAD_MAX];
static size_t s_state_payload_len;
static bool s_state_dirty;
static char s_next_payload[128];
static size_t s_next_payload_len;
static bool s_next_dirty;

static void mqtt_schedule_format_topics(void)
{
    const char *device_id = mqtt_client_device_id();

    s_state_topic[0] = '\0';
    s_next_topic[0] = '\0';

    if (device_id == NULL || device_id[0] == '\0') {
        return;
    }

    if (mqtt_topic_format(s_state_topic, sizeof(s_state_topic), device_id, "schedule/state")
            != PORT_OK) {
        s_state_topic[0] = '\0';
    }

    if (mqtt_topic_format(s_next_topic, sizeof(s_next_topic), device_id, "schedule/next")
            != PORT_OK) {
        s_next_topic[0] = '\0';
    }
}

static void mqtt_schedule_stage_payloads(void)
{
    int written;

    written = schedule_format_state_json((char *)s_state_payload, sizeof(s_state_payload));
    if (written > 0) {
        s_state_payload_len = (size_t)written;
        s_state_dirty = true;
    }

    written = schedule_format_next_json(s_next_payload, sizeof(s_next_payload));
    if (written > 0) {
        s_next_payload_len = (size_t)written;
        s_next_dirty = true;
    } else {
        s_next_payload[0] = '\0';
        s_next_payload_len = 0;
        s_next_dirty = true;
    }
}

void mqtt_schedule_request_publish(void)
{
    mqtt_schedule_format_topics();
    mqtt_schedule_stage_payloads();
}

void mqtt_schedule_connect_snapshot(void)
{
    mqtt_schedule_request_publish();
}

bool mqtt_schedule_drain(const mqtt_port_t *mqtt)
{
    bool published = false;

    if (mqtt == NULL || mqtt->publish == NULL || mqtt->is_connected == NULL ||
        !mqtt->is_connected()) {
        return false;
    }

    if (s_state_topic[0] == '\0') {
        mqtt_schedule_format_topics();
    }

    if (s_state_dirty && s_state_topic[0] != '\0' && s_state_payload_len > 0u) {
        if (mqtt->publish(s_state_topic,
                          s_state_payload,
                          s_state_payload_len,
                          1,
                          true)
                == PORT_OK) {
            s_state_dirty = false;
            published = true;
        }
    }

    if (s_next_dirty && s_next_topic[0] != '\0') {
        if (mqtt->publish(s_next_topic,
                          s_next_payload,
                          s_next_payload_len,
                          1,
                          true)
                == PORT_OK) {
            s_next_dirty = false;
            published = true;
        }
    }

    return published;
}

bool mqtt_schedule_handle(mqtt_route_kind_t route,
                          const void *payload,
                          size_t len)
{
    const char *json = payload;

    if (json == NULL || len == 0) {
        return false;
    }

    if (!schedule_cmd_apply_json(route, json, len)) {
        app_log_info("mqtt", "schedule cmd rejected route=%d", (int)route);
        return false;
    }

    mqtt_schedule_request_publish();
    return true;
}

void mqtt_schedule_test_reset(void)
{
    s_state_topic[0] = '\0';
    s_next_topic[0] = '\0';
    s_state_payload_len = 0;
    s_state_dirty = false;
    s_next_payload[0] = '\0';
    s_next_payload_len = 0;
    s_next_dirty = false;
}
