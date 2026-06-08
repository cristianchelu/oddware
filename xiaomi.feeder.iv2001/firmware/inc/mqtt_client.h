#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdbool.h>

void mqtt_client_start(void);
bool mqtt_client_wifi_is_ready(void);
bool mqtt_client_connect_in_progress(void);
bool mqtt_client_request_connect(void);
void mqtt_client_stop(void);
void mqtt_client_notify_wifi_ready(void);
void mqtt_client_suspend_for_ota(void);
void mqtt_client_resume_after_ota(void);
bool mqtt_client_wait_disconnected(uint32_t timeout_ms);

#endif /* MQTT_CLIENT_H */
