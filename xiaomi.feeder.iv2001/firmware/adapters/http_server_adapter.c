/*
 * HTTP server port adapter — spec/40-architecture/ports.md
 */

#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "syslog.h"

#include "httpd.h"

#include "http_server_adapter.h"

log_create_module(http_srv, PRINT_LEVEL_INFO);

#define HTTP_SERVER_STOP_WAIT_MS 100
/* httpd_main select() timeout is 10 s — wait longer before restart. */
#define HTTP_SERVER_STOP_WAIT_MAX 150

static QueueHandle_t s_fb_queue;
static bool s_running;

static port_err_t http_server_port_stop(void);

static void http_server_wait_stopped(void)
{
    HTTPD_STATUS status;

    for (int i = 0; i < HTTP_SERVER_STOP_WAIT_MAX; i++) {
        status = httpd_get_status();
        if (status == HTTPD_STATUS_STOP || status == HTTPD_STATUS_UNINIT) {
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(HTTP_SERVER_STOP_WAIT_MS));
    }
}

static int http_server_wait_for_run(void)
{
    httpd_fb fb;

    if (s_fb_queue == NULL) {
        return -1;
    }

    for (;;) {
        if (xQueueReceive(s_fb_queue, &fb, portMAX_DELAY) != pdPASS) {
            continue;
        }

        if (fb.status == HTTPD_STATUS_RUN) {
            return 0;
        }

        if (fb.status == HTTPD_STATUS_UNINIT) {
            return -1;
        }
    }
}

static bool http_server_ensure_stopped(void)
{
    HTTPD_STATUS status;
    HTTPD_RESULT result;

    status = httpd_get_status();
    if (status == HTTPD_STATUS_UNINIT || status == HTTPD_STATUS_STOP) {
        s_running = false;
        return true;
    }

    result = httpd_stop();
    if (result != HTTPD_RESULT_SUCCESS && result != HTTPD_RESULT_WAITING) {
        return false;
    }

    http_server_wait_stopped();
    s_running = false;
    return httpd_get_status() == HTTPD_STATUS_STOP
        || httpd_get_status() == HTTPD_STATUS_UNINIT;
}

static port_err_t http_server_do_start(uint16_t port)
{
    httpd_para parameter;
    HTTPD_RESULT result;

    (void)port;

    if (httpd_get_status() == HTTPD_STATUS_UNINIT) {
        result = httpd_init();
        if (result != HTTPD_RESULT_SUCCESS && result != HTTPD_RESULT_WAITING) {
            LOG_E(http_srv, "httpd_init failed (%d)", (int)result);
            return PORT_ERR_IO;
        }
    }

    if (s_fb_queue == NULL) {
        s_fb_queue = xQueueCreate(4, sizeof(httpd_fb));
        if (s_fb_queue == NULL) {
            return PORT_ERR_IO;
        }
    }

    memset(&parameter, 0, sizeof(parameter));
    parameter.fb_queue = s_fb_queue;
    result = httpd_start(&parameter);
    if (result == HTTPD_RESULT_WAITING) {
        if (http_server_wait_for_run() != 0) {
            LOG_E(http_srv, "httpd_start wait failed");
            s_running = false;
            return PORT_ERR_IO;
        }
    } else if (result != HTTPD_RESULT_SUCCESS) {
        LOG_E(http_srv, "httpd_start failed (%d)", (int)result);
        s_running = false;
        return PORT_ERR_IO;
    }

    s_running = true;
    LOG_I(http_srv, "HTTP server running");
    return PORT_OK;
}

static port_err_t http_server_port_start(uint16_t port)
{
    if (httpd_get_status() == HTTPD_STATUS_RUN) {
        s_running = true;
        return PORT_OK;
    }

    if (!http_server_ensure_stopped()) {
        LOG_E(http_srv, "httpd did not reach STOP before restart");
        return PORT_ERR_IO;
    }

    return http_server_do_start(port);
}

port_err_t http_server_adapter_force_restart(uint16_t port)
{
    HTTPD_STATUS status = httpd_get_status();

    if (status == HTTPD_STATUS_RUN || status == HTTPD_STATUS_STOPPING) {
        (void)http_server_ensure_stopped();
    }

    status = httpd_get_status();
    if (status != HTTPD_STATUS_STOP && status != HTTPD_STATUS_UNINIT) {
        LOG_E(http_srv, "httpd stuck in state %d", (int)status);
        return PORT_ERR_IO;
    }

    return http_server_do_start(port);
}

static port_err_t http_server_port_stop(void)
{
    if (!http_server_ensure_stopped()) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static port_err_t http_server_port_register_route(http_server_method_t method,
                                                  const char *path,
                                                  http_server_handler_t handler,
                                                  void *ctx)
{
    (void)method;
    (void)path;
    (void)handler;
    (void)ctx;
    return PORT_OK;
}

static const http_server_port_t s_http_server_port = {
    .start = http_server_port_start,
    .stop = http_server_port_stop,
    .register_route = http_server_port_register_route,
};

const http_server_port_t *http_server_port_get(void)
{
    return &s_http_server_port;
}
