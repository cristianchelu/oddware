/* Tests: spec/30-processes/uart-console.md § feed */

#include <string.h>

#include "unity.h"

#include "app_feed_cli.h"
#include "cli_test_assert.h"
#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "feed_config.h"
#include "mqtt_feed_mode.h"
#include "mqtt_outbox.h"

#define TEST_DEVICE_ID "ddeeff"

static void feed_cli_test_reset(void)
{
    fake_config_port_reset();
    fake_mqtt_port_reset();
    fake_time_reset();
    mqtt_outbox_reset();
    mqtt_feed_mode_test_reset();
    cli_test_reset();
}

void test_feed_cli_mode_show_defaults_open_loop(void)
{
    feed_cli_test_reset();
    TEST_ASSERT_EQUAL_UINT8(0, feed_cli_run_mode_show());
    assert_cli_body("feed mode: open_loop");
}

void test_feed_cli_mode_set_round_trip(void)
{
    feed_cli_test_reset();
    TEST_ASSERT_EQUAL_UINT8(0, feed_cli_run_mode_set(DISPENSE_MODE_COMPENSATED));
    assert_cli_body("feed mode ok");
    TEST_ASSERT_EQUAL(DISPENSE_MODE_COMPENSATED, feed_config_mode_get());
}

void test_feed_cli_mode_set_unchanged(void)
{
    feed_cli_test_reset();
    TEST_ASSERT_EQUAL_UINT8(1, feed_cli_run_mode_set(DISPENSE_MODE_OPEN_LOOP));
    assert_cli_body("feed mode: unchanged");
}

void test_feed_cli_mode_invalid_usage(void)
{
    char *argv[] = { "invalid" };

    feed_cli_test_reset();
    TEST_ASSERT_EQUAL_UINT8(1, feed_cli_subcmds[0].fn(1u, argv));
    assert_cli_body("usage: feed mode [open_loop|compensated]");
}

void test_feed_cli_mode_set_publishes_mqtt_when_configured(void)
{
    const fake_mqtt_port_state_t *mqtt;

    feed_cli_test_reset();
    mqtt_outbox_set_accepting(true);
    fake_mqtt_port_get()->connect(NULL);
    mqtt_feed_mode_set_device_id(TEST_DEVICE_ID);

    TEST_ASSERT_EQUAL_UINT8(0, feed_cli_run_mode_set(DISPENSE_MODE_COMPENSATED));

    while (mqtt_outbox_pending() > 0) {
        (void)mqtt_outbox_drain_one(fake_mqtt_port_get());
    }

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_topic, "feed/mode"));
    TEST_ASSERT_EQUAL_STRING("compensated", mqtt->last_publish_payload);
}
