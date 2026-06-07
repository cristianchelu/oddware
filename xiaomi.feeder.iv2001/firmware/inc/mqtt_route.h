/*
 * MQTT command topic routing — spec/30-processes/mqtt-protocol.md
 */

#ifndef MQTT_ROUTE_H
#define MQTT_ROUTE_H

typedef enum {
    MQTT_ROUTE_UNKNOWN = 0,
    MQTT_ROUTE_CMD_DISPENSE,
    MQTT_ROUTE_CMD_DISPENSE_CANCEL,
    MQTT_ROUTE_CMD_SCHEDULE_SET,
    MQTT_ROUTE_CMD_SCHEDULE_DELETE,
    MQTT_ROUTE_CMD_CALIBRATE,
    MQTT_ROUTE_CMD_DISPLAY,
    MQTT_ROUTE_CMD_CONFIG,
    MQTT_ROUTE_CMD_REBOOT,
    MQTT_ROUTE_CMD_OTA,
} mqtt_route_kind_t;

mqtt_route_kind_t mqtt_route_classify(const char *topic, const char *device_id);

#endif /* MQTT_ROUTE_H */
