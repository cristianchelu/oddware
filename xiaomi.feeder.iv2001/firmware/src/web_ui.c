/*
 * LAN web UI lifecycle — spec/30-processes/web-ui.md
 */

#include "web_ui.h"

#include <stddef.h>

#include "app_log.h"
#include "http_server_port.h"
#include "provision.h"

#if WEB_UI_ENABLE && !defined(HOST_TEST)
#include "FreeRTOS.h"
#include "task.h"
#endif

extern const uint8_t g_web_ui_gz[];
extern const size_t g_web_ui_gz_len;

static bool s_active;
static bool s_http_running;
static bool s_resume_after_ota;

#ifdef HOST_TEST
static size_t s_test_heap_free = WEB_UI_MIN_HEAP;
#endif

const uint8_t *web_ui_gz_data(void)
{
    return g_web_ui_gz;
}

size_t web_ui_gz_len(void)
{
    return g_web_ui_gz_len;
}

bool web_ui_is_active(void)
{
    return s_active;
}

#if WEB_UI_ENABLE && !defined(HOST_TEST)
static size_t web_ui_heap_free(void)
{
    return (size_t)xPortGetFreeHeapSize();
}
#else
static size_t web_ui_heap_free(void)
{
#ifdef HOST_TEST
    return s_test_heap_free;
#else
    return WEB_UI_MIN_HEAP;
#endif
}
#endif

void web_ui_start(void)
{
    const http_server_port_t *http;

#if !WEB_UI_ENABLE
    return;
#endif

    if (s_active || s_http_running) {
        return;
    }

    if (provision_is_active()) {
        return;
    }

    if (web_ui_heap_free() < WEB_UI_MIN_HEAP) {
        app_log_info("web",
                     "web ui skipped (low heap: %u < %u)",
                     (unsigned)web_ui_heap_free(),
                     (unsigned)WEB_UI_MIN_HEAP);
        return;
    }

#ifdef HOST_TEST
    s_http_running = true;
    s_active = true;
    return;
#endif

    http = http_server_port_get();
    if (http == NULL || http->start == NULL) {
        return;
    }

    if (http->start(80) != PORT_OK) {
        app_log_error("web", "failed to start HTTP server");
        return;
    }

    s_http_running = true;
    s_active = true;
    app_log_info("web",
                 "admin UI listening on port 80 (heap free %u)",
                 (unsigned)web_ui_heap_free());
}

void web_ui_stop(void)
{
#ifndef HOST_TEST
    const http_server_port_t *http;
#endif

    if (!s_http_running) {
        s_active = false;
        return;
    }

#ifndef HOST_TEST
    http = http_server_port_get();
    if (http != NULL && http->stop != NULL) {
        (void)http->stop();
    }
#endif

    s_http_running = false;
    s_active = false;
}

void web_ui_suspend_for_ota(void)
{
    s_resume_after_ota = s_active;
    web_ui_stop();
    app_log_info("web", "admin UI suspended for ota");
}

void web_ui_resume_after_ota(void)
{
    if (!s_resume_after_ota) {
        return;
    }

    s_resume_after_ota = false;
    web_ui_start();
    app_log_info("web", "admin UI resumed after ota");
}

#ifdef HOST_TEST
void web_ui_test_reset(void)
{
    s_active = false;
    s_http_running = false;
    s_resume_after_ota = false;
    s_test_heap_free = WEB_UI_MIN_HEAP;
}

void web_ui_test_set_heap_free(size_t bytes)
{
    s_test_heap_free = bytes;
}
#endif
