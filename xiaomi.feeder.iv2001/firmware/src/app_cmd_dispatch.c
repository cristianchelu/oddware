/*
 * Shared command dispatch — spec/30-processes/web-ui.md, mqtt-protocol.md
 */

#include "app_cmd_dispatch.h"

#include <string.h>

#include "dispense.h"
#include "mqtt_config.h"
#include "mqtt_dispense_cmd.h"
#include "mqtt_feed_mode.h"
#include "mqtt_feed_overfill.h"
#include "mqtt_json.h"
#include "mqtt_schedule.h"
#include "schedule.h"
#include "schedule_cmd.h"

port_err_t app_cmd_dispatch(mqtt_route_kind_t route,
                            const void *payload,
                            size_t len,
                            const char *device_id)
{
    const char *body = payload;
    unsigned grams = 0u;

    switch (route) {
    case MQTT_ROUTE_CMD_DISPENSE:
        if (body != NULL && len > 0u) {
            if (mqtt_json_find_uint(body, len, "g", &grams)) {
                if (grams >= SCHEDULE_G_MIN && grams <= SCHEDULE_G_MAX) {
                    if (dispense_submit_grams((uint8_t)grams, DISPENSE_SOURCE_MQTT)
                            == DISPENSE_SUBMIT_OK) {
                        return PORT_OK;
                    }

                    return PORT_ERR_BUSY;
                }

                return PORT_ERR_INVALID_ARG;
            }

            if (mqtt_json_has_key(body, len, "g")) {
                return PORT_ERR_INVALID_ARG;
            }
        }

        if (dispense_submit_portions((uint8_t)MQTT_DISPENSE_DEFAULT_PORTIONS,
                                     DISPENSE_SOURCE_MQTT)
                == DISPENSE_SUBMIT_OK) {
            return PORT_OK;
        }

        return PORT_ERR_BUSY;

    case MQTT_ROUTE_CMD_DISPENSE_CANCEL:
        if (dispense_is_active()) {
            return PORT_ERR_NOT_SUPPORTED;
        }

        return PORT_OK;

    case MQTT_ROUTE_CMD_CONFIG:
        return mqtt_config_handle(payload, len);

    case MQTT_ROUTE_CMD_FEED_MODE:
        return mqtt_feed_mode_handle(payload, len);

    case MQTT_ROUTE_CMD_FEED_OVERFILL:
        return mqtt_feed_overfill_handle(payload, len);

    case MQTT_ROUTE_CMD_SCHEDULE_SET:
    case MQTT_ROUTE_CMD_SCHEDULE_DELETE:
    case MQTT_ROUTE_CMD_SCHEDULE_TOGGLE:
    case MQTT_ROUTE_CMD_SCHEDULE_SKIP:
    case MQTT_ROUTE_CMD_SCHEDULE_ENABLE:
    case MQTT_ROUTE_CMD_SCHEDULE_TODAY:
        if (body == NULL || len == 0u) {
            return PORT_ERR_INVALID_ARG;
        }

        if (!schedule_cmd_apply_json(route, body, len)) {
            return PORT_ERR_INVALID_ARG;
        }

        mqtt_schedule_request_publish();
        return PORT_OK;

    default:
        (void)device_id;
        return PORT_ERR_NOT_SUPPORTED;
    }
}
