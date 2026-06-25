/*
 * MQTT dispense command — spec/30-processes/mqtt-protocol.md § HA validation slice
 */

#include "app_log.h"
#include "dispense.h"
#include "mqtt_dispense_cmd.h"
#include "mqtt_json.h"
#include "mqtt_route.h"
#include "schedule.h"

void mqtt_dispense_cmd_handle(const char *topic,
                              const void *payload,
                              size_t len,
                              const char *device_id)
{
    unsigned grams = 0u;

    if (topic == NULL || device_id == NULL || device_id[0] == '\0') {
        return;
    }

    if (mqtt_route_classify(topic, device_id) != MQTT_ROUTE_CMD_DISPENSE) {
        return;
    }

    if (payload != NULL && len > 0u) {
        const char *json = payload;

        if (mqtt_json_find_uint(json, len, "g", &grams)) {
            if (grams >= SCHEDULE_G_MIN && grams <= SCHEDULE_G_MAX) {
                (void)dispense_submit_grams((uint8_t)grams, DISPENSE_SOURCE_MQTT);
            } else {
                app_log_info("mqtt", "dispense rejected");
            }
            return;
        }

        if (mqtt_json_has_key(json, len, "g")) {
            app_log_info("mqtt", "dispense rejected");
            return;
        }
    }

    (void)dispense_submit_portions((uint8_t)MQTT_DISPENSE_DEFAULT_PORTIONS,
                                   DISPENSE_SOURCE_MQTT);
}
