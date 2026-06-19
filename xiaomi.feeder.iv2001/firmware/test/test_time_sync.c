/* Tests: spec/30-processes/time-sync.md */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "fake_time_port.h"
#include "mqtt_config.h"
#include "mqtt_outbox.h"
#include "time_sync.h"

#define TEST_EPOCH 1718841600LL

static void drain_config_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

void test_time_sync_unknown_until_ntp_success(void)
{
    fake_time_port_reset();
    fake_config_port_reset();
    mqtt_config_test_reset();
    time_sync_test_reset();

    time_sync_init();
    TEST_ASSERT_FALSE(time_sync_is_valid());

    time_sync_on_wifi_ready();
    TEST_ASSERT_TRUE(fake_time_port_sync_pending());

    fake_time_port_queue_sync_result(true, TEST_EPOCH);
    time_sync_poll(1000u);

    TEST_ASSERT_TRUE(time_sync_is_valid());
    {
        int64_t epoch = 0;

        TEST_ASSERT_TRUE(time_sync_get_utc_epoch(&epoch));
        TEST_ASSERT_EQUAL_INT64(TEST_EPOCH, epoch);
    }
}

void test_time_sync_publish_config_on_success(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_port_reset();
    fake_config_port_reset();
    mqtt_config_test_reset();
    mqtt_outbox_reset();
    fake_mqtt_port_reset();
    time_sync_test_reset();

    fake_mqtt_port_get()->connect(NULL);
    mqtt_config_set_device_id("ddeeff");

    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, TEST_EPOCH);
    time_sync_poll(1000u);
    drain_config_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/config", mqtt->last_publish_topic);
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"time_synced\":true"));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"utc_epoch\":1718841600"));
}

void test_time_sync_boot_request_logs_completion(void)
{
    fake_time_port_reset();
    time_sync_test_reset();
    app_log_test_reset();

    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, TEST_EPOCH);
    time_sync_poll(1000u);

    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "time sync ok utc=1718841600"));
}

void test_time_sync_manual_request_logs_completion(void)
{
    fake_time_port_reset();
    time_sync_test_reset();
    app_log_test_reset();

    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, TEST_EPOCH);
    time_sync_poll(1000u);
    app_log_test_reset();

    fake_time_port_queue_sync_result(true, TEST_EPOCH + 18LL);
    TEST_ASSERT_EQUAL(TIME_SYNC_REQUEST_OK, time_sync_request_now());
    time_sync_poll(2000u);

    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "time sync ok utc=1718841618"));
}

void test_time_sync_manual_request_logs_failure(void)
{
    fake_time_port_reset();
    time_sync_test_reset();
    app_log_test_reset();

    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, TEST_EPOCH);
    time_sync_poll(1000u);
    app_log_test_reset();

    fake_time_port_queue_sync_result(false, 0);
    TEST_ASSERT_EQUAL(TIME_SYNC_REQUEST_OK, time_sync_request_now());
    time_sync_poll(2000u);

    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "time sync failed"));
}

void test_time_sync_periodic_resync_logs_completion(void)
{
    fake_time_port_reset();
    time_sync_test_reset();
    app_log_test_reset();

    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, TEST_EPOCH);
    time_sync_poll(1000u);
    app_log_test_reset();

    fake_time_port_queue_sync_result(true, TEST_EPOCH + 42LL);
    time_sync_poll(1000u + (6u * 60u * 60u * 1000u));
    time_sync_poll(2000u + (6u * 60u * 60u * 1000u));

    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "time sync ok utc=1718841642"));
}

void test_time_sync_failed_resync_keeps_valid_clock(void)
{
    fake_time_port_reset();
    time_sync_test_reset();

    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, TEST_EPOCH);
    time_sync_poll(1000u);
    TEST_ASSERT_TRUE(time_sync_is_valid());

    fake_time_port_queue_sync_result(false, 0);
    time_sync_poll(1000u + (6u * 60u * 60u * 1000u));
    time_sync_poll(2000u + (6u * 60u * 60u * 1000u));

    TEST_ASSERT_TRUE(time_sync_is_valid());
}

void test_time_sync_resync_after_six_hours(void)
{
    fake_time_port_reset();
    time_sync_test_reset();

    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, TEST_EPOCH);
    time_sync_poll(1000u);
    TEST_ASSERT_FALSE(fake_time_port_sync_pending());

    time_sync_poll(1000u + (6u * 60u * 60u * 1000u));
    TEST_ASSERT_TRUE(fake_time_port_sync_pending());
}

void test_time_sync_retries_after_boot_failure(void)
{
    fake_time_port_reset();
    time_sync_test_reset();

    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(false, 0);
    time_sync_poll(1000u);
    TEST_ASSERT_FALSE(time_sync_is_valid());
    TEST_ASSERT_FALSE(fake_time_port_sync_pending());

    time_sync_poll(31000u);
    TEST_ASSERT_FALSE(fake_time_port_sync_pending());

    time_sync_poll(61000u);
    TEST_ASSERT_TRUE(fake_time_port_sync_pending());
}

void test_time_sync_request_busy_while_syncing(void)
{
    fake_time_port_reset();
    time_sync_test_reset();

    time_sync_init();
    time_sync_on_wifi_ready();
    TEST_ASSERT_EQUAL(TIME_SYNC_REQUEST_BUSY, time_sync_request_now());
}

void test_time_sync_request_no_network_without_wifi(void)
{
    fake_time_port_reset();
    time_sync_test_reset();

    time_sync_init();
    TEST_ASSERT_EQUAL(TIME_SYNC_REQUEST_NO_NETWORK, time_sync_request_now());
}
