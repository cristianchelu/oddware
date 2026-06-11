/* Tests: spec/30-processes/app-logging.md */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "fake_time.h"

static char s_sink_buf[512];
static size_t s_sink_len;

static void mirror_sink(const char *buf, size_t len, void *ctx)
{
    size_t room;

    (void)ctx;

    if (len == 0u) {
        return;
    }

    room = sizeof(s_sink_buf) - s_sink_len;
    if (len > room) {
        len = room;
    }

    memcpy(s_sink_buf + s_sink_len, buf, len);
    s_sink_len += len;
}

void test_app_log_format_timestamp_tag_and_message(void)
{
    fake_time_reset();
    fake_time_advance_ms(3661001u);
    app_log_test_reset();

    app_log_info("mqtt", "connected");

    TEST_ASSERT_EQUAL_STRING("[01:01:01.001] [mqtt] connected", app_log_test_last_line());
}

void test_app_log_tag_is_lowercase_module_name(void)
{
    fake_time_reset();
    app_log_test_reset();

    app_log_warn("wifi", "connect failed");

    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "[wifi]"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "connect failed"));
}

void test_app_log_mirror_sink_receives_full_line(void)
{
    fake_time_reset();
    app_log_test_reset();
    s_sink_len = 0;
    memset(s_sink_buf, 0, sizeof(s_sink_buf));

    app_log_set_sink(mirror_sink, NULL);
    app_log_info("cli", "host saved");
    app_log_clear_sink();

    TEST_ASSERT_GREATER_THAN(0, s_sink_len);
    TEST_ASSERT_NOT_NULL(strstr(s_sink_buf, "[cli]"));
    TEST_ASSERT_NOT_NULL(strstr(s_sink_buf, "host saved"));
    TEST_ASSERT_NOT_NULL(strstr(s_sink_buf, "\r\n"));
}

void test_app_log_clear_sink_stops_mirror(void)
{
    fake_time_reset();
    app_log_test_reset();
    s_sink_len = 0;

    app_log_set_sink(mirror_sink, NULL);
    app_log_clear_sink();
    app_log_info("app", "after clear");

    TEST_ASSERT_EQUAL(0, s_sink_len);
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "after clear"));
}

void test_app_log_debug_info_warn_error_all_emit(void)
{
    fake_time_reset();
    app_log_test_reset();

    app_log_debug("motor", "dbg");
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "dbg"));

    app_log_info("motor", "inf");
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "inf"));

    app_log_warn("motor", "wrn");
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "wrn"));

    app_log_error("motor", "err");
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "err"));
}

#if APP_LOG_LEVEL > APP_LOG_LEVEL_DEBUG
void test_app_log_debug_stripped_at_info_level(void)
{
    TEST_IGNORE_MESSAGE("APP_LOG_LEVEL > debug");
}
#else
void test_app_log_debug_stripped_at_info_level(void)
{
    fake_time_reset();
    app_log_test_reset();

    app_log_debug("ota", "chunk 1");
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "chunk 1"));
}
#endif
