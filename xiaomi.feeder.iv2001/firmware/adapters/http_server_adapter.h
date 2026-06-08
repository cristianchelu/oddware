#ifndef HTTP_SERVER_ADAPTER_H
#define HTTP_SERVER_ADAPTER_H

#include "http_server_port.h"

const http_server_port_t *http_server_port_get(void);

port_err_t http_server_adapter_force_restart(uint16_t port);

#endif /* HTTP_SERVER_ADAPTER_H */
