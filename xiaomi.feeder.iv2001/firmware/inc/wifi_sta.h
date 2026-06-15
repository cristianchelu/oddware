#ifndef WIFI_STA_H
#define WIFI_STA_H

#include <stdbool.h>

typedef enum {
    WIFI_STA_IDLE = 0,
    WIFI_STA_BUSY_CONNECT,
    WIFI_STA_BUSY_DISCONNECT,
} wifi_sta_busy_t;

void wifi_sta_start(void);
bool wifi_sta_request_connect(void);
bool wifi_sta_request_disconnect(void);
wifi_sta_busy_t wifi_sta_busy(void);

#endif /* WIFI_STA_H */
