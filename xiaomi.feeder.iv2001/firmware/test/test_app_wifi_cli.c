/* Tests: spec/30-processes/uart-console.md (`wifi disconnect`) */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "app_wifi_cli.h"
#include "fake_config_port.h"
#include "fake_wifi_port.h"
#include "wifi_sta.h"
#include "wifi_sta_test.h"

static char s_log_capture[256];
static size_t s_log_capture_len;

static void cli_log_sink(const char *buf, size_t len, void *ctx)
{
    size_t room;

    (void)ctx;

    if (len == 0u) {
        return;
    }

    room = sizeof(s_log_capture) - s_log_capture_len;
    if (len > room) {
        len = room;
    }

    memcpy(s_log_capture + s_log_capture_len, buf, len);
    s_log_capture_len += len;
}

static void cli_log_begin(void)
{
    app_log_test_reset();
    s_log_capture_len = 0u;
    memset(s_log_capture, 0, sizeof(s_log_capture));
    app_log_set_sink(cli_log_sink, NULL);
}

static void cli_log_end(void)
{
    app_log_clear_sink();
}

void test_wifi_disconnect_prints_disconnecting(void)
{
    fake_wifi_port_reset();
    wifi_sta_test_bootstrap();
    cli_log_begin();

    TEST_ASSERT_EQUAL_UINT8(0, wifi_cli_run_disconnect());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "disconnecting..."));
}

void test_wifi_disconnect_busy_when_connect_in_progress(void)
{
    fake_wifi_port_reset();
    fake_config_port_reset();
    wifi_sta_test_bootstrap();
    TEST_ASSERT_TRUE(wifi_sta_request_connect());
    cli_log_begin();

    TEST_ASSERT_EQUAL_UINT8(1, wifi_cli_run_disconnect());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "connect already in progress"));
}
