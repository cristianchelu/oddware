/*
 * MQTT command topic routing — spec/30-processes/mqtt-protocol.md
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "mqtt_route.h"
#include "mqtt_topics.h"

static bool topic_matches_cmd(const char *topic,
                              const char *device_id,
                              const char *cmd_suffix)
{
    char expected[128];
    int written;

    if (topic == NULL || device_id == NULL || cmd_suffix == NULL) {
        return false;
    }

    written = snprintf(expected,
                       sizeof(expected),
                       "%s/%s/cmd/%s",
                       MQTT_TOPIC_PREFIX,
                       device_id,
                       cmd_suffix);
    if (written < 0 || (size_t)written >= sizeof(expected)) {
        return false;
    }

    return strcmp(topic, expected) == 0;
}

mqtt_route_kind_t mqtt_route_classify(const char *topic, const char *device_id)
{
    if (topic == NULL || device_id == NULL || device_id[0] == '\0') {
        return MQTT_ROUTE_UNKNOWN;
    }

    if (topic_matches_cmd(topic, device_id, "dispense")) {
        return MQTT_ROUTE_CMD_DISPENSE;
    }
    if (topic_matches_cmd(topic, device_id, "dispense/cancel")) {
        return MQTT_ROUTE_CMD_DISPENSE_CANCEL;
    }
    if (topic_matches_cmd(topic, device_id, "schedule/set")) {
        return MQTT_ROUTE_CMD_SCHEDULE_SET;
    }
    if (topic_matches_cmd(topic, device_id, "schedule/delete")) {
        return MQTT_ROUTE_CMD_SCHEDULE_DELETE;
    }
    if (topic_matches_cmd(topic, device_id, "calibrate")) {
        return MQTT_ROUTE_CMD_CALIBRATE;
    }
    if (topic_matches_cmd(topic, device_id, "display")) {
        return MQTT_ROUTE_CMD_DISPLAY;
    }
    if (topic_matches_cmd(topic, device_id, "config")) {
        return MQTT_ROUTE_CMD_CONFIG;
    }
    if (topic_matches_cmd(topic, device_id, "reboot")) {
        return MQTT_ROUTE_CMD_REBOOT;
    }
    if (topic_matches_cmd(topic, device_id, "ota")) {
        return MQTT_ROUTE_CMD_OTA;
    }

    return MQTT_ROUTE_UNKNOWN;
}
