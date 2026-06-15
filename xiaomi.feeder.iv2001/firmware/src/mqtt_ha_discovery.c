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

static const mqtt_ha_entity_t s_ha_entities[] = {
    {
        .component = "button",
        .object_id = "dispense",
        .format_config = mqtt_ha_format_dispense_button_config,
    },
};

static bool mqtt_ha_enqueue_entity(const mqtt_ha_entity_t *entity,
                                   const char *device_id)
{
    char topic[128];
    char payload[512];
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
