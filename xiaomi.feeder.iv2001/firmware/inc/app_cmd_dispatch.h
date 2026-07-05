/*
 * Shared command dispatch — spec/30-processes/web-ui.md, mqtt-protocol.md
 */

#ifndef APP_CMD_DISPATCH_H
#define APP_CMD_DISPATCH_H

#include <stddef.h>

#include "mqtt_route.h"
#include "port_err.h"

port_err_t app_cmd_dispatch(mqtt_route_kind_t route,
                            const void *payload,
                            size_t len,
                            const char *device_id);

#endif /* APP_CMD_DISPATCH_H */
