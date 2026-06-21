/*
 * Home Assistant MQTT discovery — spec/30-processes/mqtt-protocol.md
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "mqtt_ha_discovery.h"
#include "mqtt_outbox.h"
#include "mqtt_topics.h"
#include "port_err.h"

typedef struct {
    const char *component;
    const char *object_id;
    int (*format_config)(char *buf, size_t len, const char *device_id);
} mqtt_ha_entity_t;

static int mqtt_ha_format_device_suffix(char *buf,
                                        size_t len,
                                        const char *device_id)
{
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "\"device\":{\"identifiers\":[\"petfeeder_%s\"],"
                       "\"name\":\"Pet Feeder %s\","
                       "\"manufacturer\":\"%s\","
                       "\"model\":\"%s\"}",
                       device_id,
                       device_id,
                       MQTT_HA_MANUFACTURER,
                       MQTT_HA_MODEL);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_dispense_button_config(char *buf,
                                          size_t len,
                                          const char *device_id)
{
    char cmd_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(cmd_topic, sizeof(cmd_topic), device_id, "cmd/dispense")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Dispense\","
                       "\"unique_id\":\"petfeeder_%s_dispense\","
                       "\"command_topic\":\"%s\","
                       "\"payload_press\":\"{}\","
                       "\"availability_topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\","
                       "%s}",
                       device_id,
                       cmd_topic,
                       connection_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_bowl_error_config(char *buf, size_t len, const char *device_id)
{
    char state_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(state_topic, sizeof(state_topic), device_id, "state") != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Bowl error\","
                       "\"unique_id\":\"petfeeder_%s_bowl_error\","
                       "\"state_topic\":\"%s\","
                       "\"value_template\":\"{{ value_json.bowl_error }}\","
                       "\"payload_on\":true,"
                       "\"payload_off\":false,"
                       "\"device_class\":\"problem\","
                       "\"availability_topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\","
                       "%s}",
                       device_id,
                       state_topic,
                       connection_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_bowl_weight_config(char *buf, size_t len, const char *device_id)
{
    char bowl_weight_topic[96];
    char state_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(bowl_weight_topic,
                          sizeof(bowl_weight_topic),
                          device_id,
                          "bowl_weight")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(state_topic, sizeof(state_topic), device_id, "state") != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Bowl weight\","
                       "\"unique_id\":\"petfeeder_%s_bowl_weight\","
                       "\"state_topic\":\"%s\","
                       "\"unit_of_measurement\":\"g\","
                       "\"device_class\":\"weight\","
                       "\"state_class\":\"measurement\","
                       "\"availability\":["
                       "{\"topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\"},"
                       "{\"topic\":\"%s\","
                       "\"value_template\":\"{{ value_json.bowl_error == false }}\","
                       "\"payload_available\":\"true\","
                       "\"payload_not_available\":\"false\"}"
                       "],"
                       "%s}",
                       device_id,
                       bowl_weight_topic,
                       connection_topic,
                       state_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_battery_config(char *buf, size_t len, const char *device_id)
{
    char battery_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(battery_topic,
                          sizeof(battery_topic),
                          device_id,
                          "battery")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Battery\","
                       "\"unique_id\":\"petfeeder_%s_battery\","
                       "\"state_topic\":\"%s\","
                       "\"unit_of_measurement\":\"%%\","
                       "\"device_class\":\"battery\","
                       "\"state_class\":\"measurement\","
                       "\"availability\":["
                       "{\"topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\"},"
                       "{\"topic\":\"%s\","
                       "\"value_template\":\"{{ 'true' if value != 'unknown' else 'false' }}\","
                       "\"payload_available\":\"true\","
                       "\"payload_not_available\":\"false\"}"
                       "],"
                       "%s}",
                       device_id,
                       battery_topic,
                       connection_topic,
                       battery_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_battery_voltage_config(char *buf, size_t len, const char *device_id)
{
    char voltage_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(voltage_topic,
                          sizeof(voltage_topic),
                          device_id,
                          "battery_voltage")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Battery pack voltage\","
                       "\"unique_id\":\"petfeeder_%s_battery_voltage\","
                       "\"state_topic\":\"%s\","
                       "\"unit_of_measurement\":\"mV\","
                       "\"device_class\":\"voltage\","
                       "\"state_class\":\"measurement\","
                       "\"enabled_by_default\":false,"
                       "\"availability_topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\","
                       "%s}",
                       device_id,
                       voltage_topic,
                       connection_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_mains_config(char *buf, size_t len, const char *device_id)
{
    char mains_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(mains_topic, sizeof(mains_topic), device_id, "mains")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Mains connected\","
                       "\"unique_id\":\"petfeeder_%s_mains\","
                       "\"state_topic\":\"%s\","
                       "\"payload_on\":\"ON\","
                       "\"payload_off\":\"OFF\","
                       "\"device_class\":\"power\","
                       "\"availability_topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\","
                       "%s}",
                       device_id,
                       mains_topic,
                       connection_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_hopper_level_config(char *buf, size_t len, const char *device_id)
{
    char hopper_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(hopper_topic, sizeof(hopper_topic), device_id, "hopper")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Hopper level\","
                       "\"unique_id\":\"petfeeder_%s_hopper_level\","
                       "\"state_topic\":\"%s\","
                       "\"device_class\":\"enum\","
                       "\"options\":[\"normal\",\"low\",\"empty\"],"
                       "\"availability_topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\","
                       "%s}",
                       device_id,
                       hopper_topic,
                       connection_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_dispense_completed_config(char *buf,
                                             size_t len,
                                             const char *device_id)
{
    char event_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(event_topic,
                          sizeof(event_topic),
                          device_id,
                          "dispense/event")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Dispense completed\","
                       "\"unique_id\":\"petfeeder_%s_dispense_completed\","
                       "\"state_topic\":\"%s\","
                       "\"event_types\":[\"success\",\"underfill\",\"stuck\",\"empty_hopper\",\"aborted\"],"
                       "\"availability_topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\","
                       "%s}",
                       device_id,
                       event_topic,
                       connection_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_device_timezone_config(char *buf, size_t len, const char *device_id)
{
    char timezone_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(timezone_topic,
                          sizeof(timezone_topic),
                          device_id,
                          "timezone")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Device timezone\","
                       "\"unique_id\":\"petfeeder_%s_device_timezone\","
                       "\"state_topic\":\"%s\","
                       "\"availability_topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\","
                       "%s}",
                       device_id,
                       timezone_topic,
                       connection_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

int mqtt_ha_format_feeding_schedule_config(char *buf, size_t len, const char *device_id)
{
    char schedule_topic[96];
    char connection_topic[96];
    char device_block[192];
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return -1;
    }

    if (mqtt_topic_format(schedule_topic,
                          sizeof(schedule_topic),
                          device_id,
                          "schedule/state")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_topic_format(connection_topic,
                          sizeof(connection_topic),
                          device_id,
                          "connection")
            != PORT_OK) {
        return -1;
    }
    if (mqtt_ha_format_device_suffix(device_block,
                                     sizeof(device_block),
                                     device_id)
            < 0) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"name\":\"Feeding schedule\","
                       "\"unique_id\":\"petfeeder_%s_feeding_schedule\","
                       "\"state_topic\":\"%s\","
                       "\"value_template\":\"{{ value_json.enabled }}\","
                       "\"payload_on\":true,"
                       "\"payload_off\":false,"
                       "\"json_attributes_topic\":\"%s\","
                       "\"json_attributes_template\":\"{{ value_json | tojson }}\","
                       "\"force_update\":true,"
                       "\"availability_topic\":\"%s\","
                       "\"payload_available\":\"online\","
                       "\"payload_not_available\":\"offline\","
                       "%s}",
                       device_id,
                       schedule_topic,
                       schedule_topic,
                       connection_topic,
                       device_block);
    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

static const mqtt_ha_entity_t s_ha_entities[] = {
    {
        .component = "button",
        .object_id = "dispense",
        .format_config = mqtt_ha_format_dispense_button_config,
    },
    {
        .component = "binary_sensor",
        .object_id = "bowl_error",
        .format_config = mqtt_ha_format_bowl_error_config,
    },
    {
        .component = "sensor",
        .object_id = "bowl_weight",
        .format_config = mqtt_ha_format_bowl_weight_config,
    },
    {
        .component = "sensor",
        .object_id = "battery",
        .format_config = mqtt_ha_format_battery_config,
    },
    {
        .component = "sensor",
        .object_id = "battery_voltage",
        .format_config = mqtt_ha_format_battery_voltage_config,
    },
    {
        .component = "binary_sensor",
        .object_id = "mains",
        .format_config = mqtt_ha_format_mains_config,
    },
    {
        .component = "sensor",
        .object_id = "hopper_level",
        .format_config = mqtt_ha_format_hopper_level_config,
    },
    {
        .component = "sensor",
        .object_id = "device_timezone",
        .format_config = mqtt_ha_format_device_timezone_config,
    },
    {
        .component = "binary_sensor",
        .object_id = "feeding_schedule",
        .format_config = mqtt_ha_format_feeding_schedule_config,
    },
    {
        .component = "event",
        .object_id = "dispense_completed",
        .format_config = mqtt_ha_format_dispense_completed_config,
    },
};

static bool mqtt_ha_enqueue_entity(const mqtt_ha_entity_t *entity,
                                   const char *device_id)
{
    char topic[128];
    char payload[768];
    int written;

    if (entity == NULL || device_id == NULL || device_id[0] == '\0') {
        return false;
    }

    written = snprintf(topic,
                       sizeof(topic),
                       "homeassistant/%s/petfeeder_%s/%s/config",
                       entity->component,
                       device_id,
                       entity->object_id);
    if (written < 0 || (size_t)written >= sizeof(topic)) {
        return false;
    }

    written = entity->format_config(payload, sizeof(payload), device_id);
    if (written < 0) {
        return false;
    }

    if (!mqtt_outbox_enqueue(topic, payload, (size_t)written, 1, true)) {
        app_log_debug("mqtt", "ha discovery enqueue failed topic=%s", topic);
        return false;
    }

    return true;
}

void mqtt_ha_discovery_schedule(const char *device_id)
{
    size_t i;

    if (device_id == NULL || device_id[0] == '\0') {
        return;
    }

    for (i = 0; i < (sizeof(s_ha_entities) / sizeof(s_ha_entities[0])); i++) {
        (void)mqtt_ha_enqueue_entity(&s_ha_entities[i], device_id);
    }
}
