/*
 * MQTT command dispatch from app — spec/30-processes/app-event-loop.md
 */

#include <stdio.h>

#include "app_mqtt_dispatch.h"
#include "mqtt_route.h"
#include "ota_client.h"

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
    default:
        printf("[app] mqtt cmd stub route=%d topic=%s\r\n", (int)route, topic);
        break;
    }
}
