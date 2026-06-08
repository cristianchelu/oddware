/*
 * HTTP server port — spec/40-architecture/ports.md
 */

#ifndef HTTP_SERVER_PORT_H
#define HTTP_SERVER_PORT_H

#include <stddef.h>
#include <stdint.h>

#include "port_err.h"

typedef enum http_server_method {
    HTTP_SERVER_GET = 0,
    HTTP_SERVER_POST,
} http_server_method_t;

typedef struct http_server_request {
    http_server_method_t method;
    const char *path;
    const char *body;
    size_t body_len;
} http_server_request_t;

typedef struct http_server_response {
    int status_code;
    const char *content_type;
    const char *body;
    size_t body_len;
} http_server_response_t;

typedef void (*http_server_handler_t)(const http_server_request_t *req,
                                      http_server_response_t *resp,
                                      void *ctx);

typedef struct http_server_port {
    port_err_t (*start)(uint16_t port);
    port_err_t (*stop)(void);
    port_err_t (*register_route)(http_server_method_t method,
                                 const char *path,
                                 http_server_handler_t handler,
                                 void *ctx);
} http_server_port_t;

const http_server_port_t *http_server_port_get(void);

#endif /* HTTP_SERVER_PORT_H */
