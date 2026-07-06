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
    if (topic_matches_cmd(topic, device_id, "schedule/toggle")) {
        return MQTT_ROUTE_CMD_SCHEDULE_TOGGLE;
    }
    if (topic_matches_cmd(topic, device_id, "schedule/skip")) {
        return MQTT_ROUTE_CMD_SCHEDULE_SKIP;
    }
    if (topic_matches_cmd(topic, device_id, "schedule/enable")) {
        return MQTT_ROUTE_CMD_SCHEDULE_ENABLE;
    }
    if (topic_matches_cmd(topic, device_id, "schedule/today")) {
        return MQTT_ROUTE_CMD_SCHEDULE_TODAY;
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
    if (topic_matches_cmd(topic, device_id, "feed/mode")) {
        return MQTT_ROUTE_CMD_FEED_MODE;
    }
    if (topic_matches_cmd(topic, device_id, "feed/overfill")) {
        return MQTT_ROUTE_CMD_FEED_OVERFILL;
    }
    if (topic_matches_cmd(topic, device_id, "reboot")) {
        return MQTT_ROUTE_CMD_REBOOT;
    }
    if (topic_matches_cmd(topic, device_id, "ota")) {
        return MQTT_ROUTE_CMD_OTA;
    }

    return MQTT_ROUTE_UNKNOWN;
}

const char *mqtt_route_label(mqtt_route_kind_t route)
{
    switch (route) {
    case MQTT_ROUTE_CMD_DISPENSE:
        return "dispense";
    case MQTT_ROUTE_CMD_DISPENSE_CANCEL:
        return "dispense_cancel";
    case MQTT_ROUTE_CMD_SCHEDULE_SET:
        return "schedule_set";
    case MQTT_ROUTE_CMD_SCHEDULE_DELETE:
        return "schedule_delete";
    case MQTT_ROUTE_CMD_SCHEDULE_TOGGLE:
        return "schedule_toggle";
    case MQTT_ROUTE_CMD_SCHEDULE_SKIP:
        return "schedule_skip";
    case MQTT_ROUTE_CMD_SCHEDULE_ENABLE:
        return "schedule_enable";
    case MQTT_ROUTE_CMD_SCHEDULE_TODAY:
        return "schedule_today";
    case MQTT_ROUTE_CMD_CALIBRATE:
        return "calibrate";
    case MQTT_ROUTE_CMD_DISPLAY:
        return "display";
    case MQTT_ROUTE_CMD_CONFIG:
        return "config";
    case MQTT_ROUTE_CMD_FEED_MODE:
        return "feed_mode";
    case MQTT_ROUTE_CMD_FEED_OVERFILL:
        return "feed_overfill";
    case MQTT_ROUTE_CMD_REBOOT:
        return "reboot";
    case MQTT_ROUTE_CMD_OTA:
        return "ota";
    default:
        return "unknown";
    }
}
