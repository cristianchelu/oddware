#ifndef WIFI_STA_H
#define WIFI_STA_H

#include <stdbool.h>

void wifi_sta_start(void);
bool wifi_sta_request_connect(void);

#endif /* WIFI_STA_H */
