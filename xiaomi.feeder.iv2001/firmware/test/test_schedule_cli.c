/* Tests: spec/30-processes/uart-console.md § schedule commands */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "app_schedule_cli.h"
#include "fake_config_port.h"
#include "fake_time_port.h"
#include "schedule.h"
#include "schedule_test_epochs.h"
#include "time_sync.h"
#include "tz_rule.h"

static char s_log_capture[1024];
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

static void setup_schedule_synced(int64_t epoch)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    fake_time_port_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    schedule_test_reset();
    schedule_set_changed_fn(NULL);

    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg, "UTC0"));
    tz_rule_init();

    fake_time_port_set_epoch(epoch);
    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, epoch);
    time_sync_poll(1000u);
    TEST_ASSERT_TRUE(time_sync_is_valid());

    schedule_init();
}

void test_schedule_cli_show_empty(void)
{
    cli_log_begin();
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_show());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule: enabled=on today=on slots=0"));
}

void test_schedule_cli_show_lists_slot(void)
{
    schedule_slot_config_t slot = {
        .hour = 9,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };

    cli_log_begin();
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));

    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_show());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "slots=1"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "09:00"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "days=127"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "state=pending"));
}

void test_schedule_cli_set_and_delete_round_trip(void)
{
    cli_log_begin();
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_set(9, 30, 127, 25, true));
    TEST_ASSERT_EQUAL_size_t(1, schedule_slot_count());
    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_delete(9, 30));
    TEST_ASSERT_EQUAL_size_t(0, schedule_slot_count());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule set ok"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule delete ok"));
}

void test_schedule_cli_delete_missing_slot(void)
{
    cli_log_begin();
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_EQUAL_UINT8(1, schedule_cli_run_delete(12, 0));
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule: slot not found"));
}

void test_schedule_cli_enable_and_today_reflected_in_show(void)
{
    cli_log_begin();
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_enable(false));
    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_today(false));
    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_show());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule enable ok"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule today ok"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "enabled=off today=off"));
}

void test_schedule_cli_skip_updates_show_line(void)
{
    schedule_slot_config_t slot = {
        .hour = 10,
        .min = 0,
        .days = 127,
        .g = 20,
        .enabled = true,
    };

    cli_log_begin();
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_skip(10, 0, true));
    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_show());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule skip ok"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "skip=on"));
}

void test_schedule_cli_next_when_upcoming(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };

    cli_log_begin();
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));

    TEST_ASSERT_EQUAL_UINT8(0, schedule_cli_run_next());
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule next: 08:00 30g"));
}

void test_schedule_cli_set_invalid_usage(void)
{
    cli_log_begin();
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_EQUAL_UINT8(1, schedule_cli_run_set(8, 0, 127, 4, true));
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule: invalid g"));
}

void test_schedule_cli_enable_unchanged(void)
{
    cli_log_begin();
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_EQUAL_UINT8(1, schedule_cli_run_enable(true));
    cli_log_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "schedule: unchanged"));
}
