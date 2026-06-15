/*
 * MQTT dispense command — spec/30-processes/mqtt-protocol.md § HA validation slice
 */

#include "app_log.h"
#include "dispense.h"
#include "mqtt_dispense_cmd.h"
#include "mqtt_route.h"

void mqtt_dispense_cmd_handle(const char *topic,
                              const void *payload,
                              size_t len,
                              const char *device_id)
{
    dispense_submit_result_t result;

    (void)payload;
    (void)len;

    if (topic == NULL || device_id == NULL || device_id[0] == '\0') {
        return;
    }

    if (mqtt_route_classify(topic, device_id) != MQTT_ROUTE_CMD_DISPENSE) {
        return;
    }

    app_log_info("dispense", "remote dispense cmd topic=%s len=%u", topic, (unsigned)len);

    result = dispense_submit_portions((uint8_t)MQTT_DISPENSE_DEFAULT_PORTIONS);
    if (result == DISPENSE_SUBMIT_OK) {
        app_log_info("dispense",
                     "remote dispense accepted portions=%u",
                     (unsigned)MQTT_DISPENSE_DEFAULT_PORTIONS);
    } else if (result == DISPENSE_SUBMIT_BUSY) {
        app_log_info("dispense", "remote dispense busy");
    } else {
        app_log_info("dispense", "remote dispense rejected result=%d", (int)result);
    }
}
