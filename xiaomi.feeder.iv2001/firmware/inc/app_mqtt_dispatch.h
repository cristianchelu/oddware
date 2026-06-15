#ifndef APP_MQTT_DISPATCH_H
#define APP_MQTT_DISPATCH_H

#include <stddef.h>

void app_mqtt_on_connected(void);

void app_mqtt_dispatch(const char *topic,
                       const void *payload,
                       size_t len,
                       const char *device_id);

#endif /* APP_MQTT_DISPATCH_H */
