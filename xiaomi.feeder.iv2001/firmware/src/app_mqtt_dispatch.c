/*
 * MQTT command dispatch from app — spec/30-processes/app-event-loop.md
 */

#include "app_log.h"
#include "app_mqtt_dispatch.h"
#include "battery_monitor.h"
#include "mqtt_client.h"
#include "mqtt_dispense_cmd.h"
#include "mqtt_config.h"
#include "mqtt_feed_mode.h"
#include "mqtt_ha_discovery.h"
#include "mqtt_route.h"
#include "mqtt_state.h"
#include "mqtt_bowl_weight.h"
#include "mqtt_battery.h"
#include "mqtt_battery_voltage.h"
#include "mqtt_hopper.h"
#include "mqtt_mains.h"
#include "mqtt_timezone.h"
#include "mqtt_schedule.h"
#include "schedule.h"
#include "hopper_level.h"
#include "ota_client.h"
#include "power_source_input.h"
#include "port_err.h"

#include "FreeRTOS.h"
#include "task.h"

void app_mqtt_on_connected(void)
{
    const char *device_id = mqtt_client_device_id();

    ota_client_on_mqtt_connected();
    mqtt_state_on_mqtt_connected();
    mqtt_bowl_weight_on_mqtt_connected();
    mqtt_hopper_connect_snapshot(hopper_level_get());
    if (power_source_input_is_valid()) {
        mqtt_mains_connect_snapshot(power_source_input_get() == POWER_SOURCE_MAINS);
    }
    battery_monitor_force_sample();
    if (!battery_monitor_poll(
            (uint32_t)(xTaskGetTickCount() * (TickType_t)portTICK_PERIOD_MS))) {
        mqtt_battery_on_mqtt_connected();
        mqtt_battery_voltage_on_mqtt_connected();
    }
    if (device_id != NULL && device_id[0] != '\0') {
        mqtt_ha_discovery_schedule(device_id);
    }
    mqtt_timezone_connect_snapshot();
    mqtt_schedule_connect_snapshot();
    mqtt_config_connect_snapshot();
    mqtt_feed_mode_connect_snapshot();
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
        app_log_info("mqtt",
                     "cmd %s topic=%s len=%u",
                     mqtt_route_label(route),
                     topic,
                     (unsigned)len);
        ota_client_on_mqtt_message(topic, payload, len);
        break;
    case MQTT_ROUTE_CMD_DISPENSE:
        app_log_info("mqtt",
                     "cmd %s topic=%s len=%u",
                     mqtt_route_label(route),
                     topic,
                     (unsigned)len);
        mqtt_dispense_cmd_handle(topic, payload, len, device_id);
        break;
    case MQTT_ROUTE_CMD_CONFIG:
        app_log_info("mqtt",
                     "cmd %s topic=%s len=%u",
                     mqtt_route_label(route),
                     topic,
                     (unsigned)len);
        if (mqtt_config_handle(payload, len) != PORT_OK) {
            app_log_info("mqtt", "config rejected");
        }
        break;
    case MQTT_ROUTE_CMD_FEED_MODE:
        app_log_info("mqtt",
                     "cmd %s topic=%s len=%u",
                     mqtt_route_label(route),
                     topic,
                     (unsigned)len);
        if (mqtt_feed_mode_handle(payload, len) != PORT_OK) {
            app_log_info("mqtt", "feed mode rejected");
        }
        break;
    case MQTT_ROUTE_CMD_SCHEDULE_SET:
    case MQTT_ROUTE_CMD_SCHEDULE_DELETE:
    case MQTT_ROUTE_CMD_SCHEDULE_TOGGLE:
    case MQTT_ROUTE_CMD_SCHEDULE_SKIP:
    case MQTT_ROUTE_CMD_SCHEDULE_ENABLE:
    case MQTT_ROUTE_CMD_SCHEDULE_TODAY:
        app_log_info("mqtt",
                     "cmd %s topic=%s len=%u",
                     mqtt_route_label(route),
                     topic,
                     (unsigned)len);
        (void)mqtt_schedule_handle(route, payload, len);
        break;
    default:
        app_log_debug("app", "mqtt cmd stub route=%d topic=%s", (int)route, topic);
        break;
    }
}
