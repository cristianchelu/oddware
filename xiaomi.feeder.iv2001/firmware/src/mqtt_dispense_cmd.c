/*
 * MQTT dispense command — spec/30-processes/mqtt-protocol.md § HA validation slice
 */

#include "dispense.h"
#include "mqtt_dispense_cmd.h"
#include "mqtt_route.h"

void mqtt_dispense_cmd_handle(const char *topic,
                              const void *payload,
                              size_t len,
                              const char *device_id)
{
    (void)payload;
    (void)len;

    if (topic == NULL || device_id == NULL || device_id[0] == '\0') {
        return;
    }

    if (mqtt_route_classify(topic, device_id) != MQTT_ROUTE_CMD_DISPENSE) {
        return;
    }

    (void)dispense_submit_portions((uint8_t)MQTT_DISPENSE_DEFAULT_PORTIONS,
                                   DISPENSE_SOURCE_MQTT);
}
