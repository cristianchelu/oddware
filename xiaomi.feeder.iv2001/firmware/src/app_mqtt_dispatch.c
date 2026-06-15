/*
 * MQTT command dispatch from app — spec/30-processes/app-event-loop.md
 */

#include "app_log.h"
#include "app_mqtt_dispatch.h"
#include "mqtt_client.h"
#include "mqtt_dispense_cmd.h"
#include "mqtt_ha_discovery.h"
#include "mqtt_route.h"
#include "ota_client.h"

void app_mqtt_on_connected(void)
{
    const char *device_id = mqtt_client_device_id();

    ota_client_on_mqtt_connected();
    if (device_id != NULL && device_id[0] != '\0') {
        mqtt_ha_discovery_schedule(device_id);
    }
}

void app_mqtt_dispatch(const char *topic,
                       const void *payload,
                       size_t len,
                       const char *device_id)
{
    mqtt_route_kind_t route;

    if (topic == NULL || payload == NULL || device_id == NULL || device_id[0] == '\0') {
        return;
    }

    route = mqtt_route_classify(topic, device_id);

    switch (route) {
    case MQTT_ROUTE_CMD_OTA:
        ota_client_on_mqtt_message(topic, payload, len);
        break;
    case MQTT_ROUTE_CMD_DISPENSE:
        mqtt_dispense_cmd_handle(topic, payload, len, device_id);
        break;
    default:
        app_log_debug("app", "mqtt cmd stub route=%d topic=%s", (int)route, topic);
        break;
    }
}
