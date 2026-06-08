#ifndef OTA_CLIENT_H
#define OTA_CLIENT_H

#include <stddef.h>

void ota_client_start(void);
void ota_client_set_device_id(const char *device_id);
void ota_client_on_mqtt_message(const char *topic, const void *payload, size_t len);
void ota_client_on_mqtt_connected(void);
uint32_t ota_client_poll_ms(void);

#endif /* OTA_CLIENT_H */
