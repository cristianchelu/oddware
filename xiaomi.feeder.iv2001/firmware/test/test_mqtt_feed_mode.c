/* Tests: spec/30-processes/mqtt-protocol.md § Feed mode */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "feed_config.h"
#include "mqtt_feed_mode.h"
#include "mqtt_outbox.h"

#define TEST_DEVICE_ID "ddeeff"

static void drain_one_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    if (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void drain_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void setup_feed_mode_mqtt(void)
{
    fake_config_port_reset();
    fake_mqtt_port_reset();
    fake_time_reset();
    mqtt_outbox_reset();
    mqtt_feed_mode_test_reset();
    mqtt_outbox_set_accepting(true);
    fake_mqtt_port_get()->connect(NULL);
    mqtt_feed_mode_set_device_id(TEST_DEVICE_ID);
}

void test_mqtt_feed_mode_connect_snapshot_publishes_open_loop(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_feed_mode_mqtt();
    mqtt_feed_mode_connect_snapshot();
    drain_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/feed/mode", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("open_loop", mqtt->last_publish_payload);
    TEST_ASSERT_TRUE(mqtt->last_publish_retain);
}

void test_mqtt_feed_mode_apply_publishes_on_change(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_feed_mode_mqtt();
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_feed_mode_apply(DISPENSE_MODE_COMPENSATED));
    drain_one_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/feed/mode", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("compensated", mqtt->last_publish_payload);
    TEST_ASSERT_EQUAL(DISPENSE_MODE_COMPENSATED, feed_config_mode_get());
    TEST_ASSERT_EQUAL_UINT(0, mqtt_outbox_pending());
}

void test_mqtt_feed_mode_handle_plain_text(void)
{
    setup_feed_mode_mqtt();
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_feed_mode_handle("compensated", strlen("compensated")));
    TEST_ASSERT_EQUAL(DISPENSE_MODE_COMPENSATED, feed_config_mode_get());
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_feed_mode_handle("open_loop", strlen("open_loop")));
    TEST_ASSERT_EQUAL(DISPENSE_MODE_OPEN_LOOP, feed_config_mode_get());
}

void test_mqtt_feed_mode_handle_rejects_invalid_payload(void)
{
    setup_feed_mode_mqtt();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      mqtt_feed_mode_handle("invalid", strlen("invalid")));
}

void test_mqtt_feed_mode_apply_unchanged_skips_publish(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_feed_mode_mqtt();
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_feed_mode_apply(DISPENSE_MODE_OPEN_LOOP));
    drain_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->publish_calls);
}
