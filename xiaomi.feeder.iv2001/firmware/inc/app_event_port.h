#ifndef APP_EVENT_PORT_H
#define APP_EVENT_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "app_event.h"

void app_event_port_init(void);
bool app_event_receive(app_event_t *ev, uint32_t wait_ms);
bool app_event_try_receive(app_event_t *ev);

#endif /* APP_EVENT_PORT_H */
