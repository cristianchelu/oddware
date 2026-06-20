/* Tests: spec/30-processes/time-sync.md */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "fake_time_port.h"
#include "time_local.h"
#include "time_sync.h"
#include "tz_rule.h"

#define TEST_EPOCH 1718841600LL

static void setup_synced_with_posix(const char *posix, int64_t epoch)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    fake_time_port_reset();
    time_sync_test_reset();
    tz_rule_test_reset();

    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg, posix));
    tz_rule_init();

    fake_time_port_set_epoch(epoch);
    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, epoch);
    time_sync_poll(1000u);
    TEST_ASSERT_TRUE(time_sync_is_valid());
}

void test_time_local_from_utc_utc_midnight(void)
{
    time_local_t local;

    TEST_ASSERT_TRUE(time_local_from_utc(0, 0, &local));
    TEST_ASSERT_EQUAL_UINT16(1970, local.year);
    TEST_ASSERT_EQUAL_UINT8(1, local.month);
    TEST_ASSERT_EQUAL_UINT8(1, local.day);
    TEST_ASSERT_EQUAL_UINT8(0, local.hour);
    TEST_ASSERT_EQUAL_UINT8(0, local.min);
    TEST_ASSERT_EQUAL_UINT8(0, local.sec);
    TEST_ASSERT_EQUAL_UINT8(3, local.wday_mon0);
}

void test_time_local_from_utc_fixed_offset(void)
{
    time_local_t local;

    TEST_ASSERT_TRUE(time_local_from_utc(0, 480, &local));
    TEST_ASSERT_EQUAL_UINT8(8, local.hour);
}

void test_time_local_from_utc_negative_offset(void)
{
    time_local_t local;

    TEST_ASSERT_TRUE(time_local_from_utc(3600, -300, &local));
    TEST_ASSERT_EQUAL_UINT8(20, local.hour);
}

void test_time_local_now_fixed_offset(void)
{
    time_local_t local;

    setup_synced_with_posix("CST-8", TEST_EPOCH);
    TEST_ASSERT_TRUE(time_local_now(&local));
    TEST_ASSERT_EQUAL_UINT8(8, local.hour);
}

void test_time_local_now_us_eastern_winter(void)
{
    time_local_t local;

    setup_synced_with_posix("EST5EDT,M3.2.0,M11.1.0", 1705320000LL);
    TEST_ASSERT_TRUE(time_local_now(&local));
    TEST_ASSERT_EQUAL_UINT16(2024, local.year);
    TEST_ASSERT_EQUAL_UINT8(1, local.month);
    TEST_ASSERT_EQUAL_UINT8(15, local.day);
    TEST_ASSERT_EQUAL_UINT8(7, local.hour);
}

void test_time_local_now_false_when_not_synced(void)
{
    time_local_t local;

    fake_time_port_reset();
    fake_config_port_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    tz_rule_init();
    time_sync_init();

    TEST_ASSERT_FALSE(time_local_now(&local));
}
