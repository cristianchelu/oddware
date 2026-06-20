/* Tests: spec/30-processes/uart-console.md § time commands */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "app_time_cli.h"
#include "fake_config_port.h"
#include "fake_time_port.h"
#include "time_sync.h"
#include "tz_rule.h"

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

static void setup_synced_clock(int64_t epoch)
{
    fake_time_port_reset();
    fake_config_port_reset();
    time_sync_test_reset();

    fake_time_port_set_epoch(epoch);
    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, epoch);
    time_sync_poll(1000u);
}

void test_time_cli_show_not_synced_includes_tz_rule(void)
{
    fake_config_port_reset();
    time_sync_test_reset();
    cli_log_begin();

    TEST_ASSERT_EQUAL_UINT8(0, time_cli_run_show());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "time: not synced"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "tz_rule=UTC0"));
}

void test_time_cli_show_synced_includes_local_time(void)
{
    cli_log_begin();
    setup_synced_clock(1718841600LL);
    TEST_ASSERT_EQUAL_UINT8(0, time_cli_run_show());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "time: synced utc=1718841600"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "local="));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "wday="));
}

void test_time_cli_show_synced_echoes_stored_posix(void)
{
    const config_port_t *cfg = fake_config_port_get();
    static const char posix[] = "EET-2EEST,M3.5.0/3,M10.5.0/4";

    cli_log_begin();
    setup_synced_clock(1718841600LL);
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg, posix));
    tz_rule_init();
    TEST_ASSERT_EQUAL_UINT8(0, time_cli_run_show());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "tz_rule=EET-2EEST,M3.5.0/3,M10.5.0/4"));
}

void test_time_cli_sync_started(void)
{
    fake_time_port_reset();
    time_sync_test_reset();
    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, 1718841600LL);
    time_sync_poll(1000u);
    cli_log_begin();

    TEST_ASSERT_EQUAL_UINT8(0, time_cli_run_sync());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "time sync started"));
}

void test_time_cli_sync_busy_while_syncing(void)
{
    fake_time_port_reset();
    time_sync_test_reset();
    time_sync_init();
    time_sync_on_wifi_ready();
    cli_log_begin();

    TEST_ASSERT_EQUAL_UINT8(1, time_cli_run_sync());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "time sync busy"));
}

void test_time_cli_sync_no_network_without_wifi(void)
{
    fake_time_port_reset();
    time_sync_test_reset();
    time_sync_init();
    cli_log_begin();

    TEST_ASSERT_EQUAL_UINT8(1, time_cli_run_sync());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "time sync: no network"));
}

void test_time_cli_set_tz_rule_saved(void)
{
    cli_log_begin();
    fake_config_port_reset();
    tz_rule_test_reset();
    TEST_ASSERT_EQUAL_UINT8(0,
                            time_cli_run_set_tz_rule("EET-2EEST,M3.5.0/3,M10.5.0/4"));
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "tz_rule saved"));
}

void test_time_cli_set_tz_rule_invalid_posix(void)
{
    cli_log_begin();
    fake_config_port_reset();
    tz_rule_test_reset();
    TEST_ASSERT_EQUAL_UINT8(1, time_cli_run_set_tz_rule("480"));
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "invalid tz_rule"));
}

void test_time_cli_set_tz_rule_usage(void)
{
    cli_log_begin();
    TEST_ASSERT_EQUAL_UINT8(1, time_cli_run_set_tz_rule(NULL));
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "usage: time set tz_rule <posix>"));
}

void test_time_cli_set_tz_label_saved(void)
{
    cli_log_begin();
    fake_config_port_reset();
    TEST_ASSERT_EQUAL_UINT8(0, time_cli_run_set_tz_label("Europe/Bucharest"));
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "tz_label saved"));
}

void test_time_cli_set_tz_label_invalid(void)
{
    cli_log_begin();
    fake_config_port_reset();
    TEST_ASSERT_EQUAL_UINT8(1,
                            time_cli_run_set_tz_label(
                                "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMN"));
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "invalid tz_label"));
}
