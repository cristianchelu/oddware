/* Tests: spec/30-processes/mqtt-protocol.md § Overfill protection */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "feed_config.h"
#include "mqtt_feed_overfill.h"
#include "mqtt_outbox.h"

#define TEST_DEVICE_ID "ddeeff"

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

static void setup_feed_overfill_mqtt(void)
{
    fake_config_port_reset();
    fake_mqtt_port_reset();
    fake_time_reset();
    mqtt_outbox_reset();
    mqtt_feed_overfill_test_reset();
    mqtt_outbox_set_accepting(true);
    fake_mqtt_port_get()->connect(NULL);
    mqtt_feed_overfill_set_device_id(TEST_DEVICE_ID);
}

void test_mqtt_feed_overfill_connect_snapshot_publishes_defaults(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_feed_overfill_mqtt();
    mqtt_feed_overfill_connect_snapshot();
    drain_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/feed/overfill", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("{\"enabled\":false,\"threshold_g\":50}", mqtt->last_publish_payload);
    TEST_ASSERT_TRUE(mqtt->last_publish_retain);
}

void test_mqtt_feed_overfill_handle_partial_enabled(void)
{
    setup_feed_overfill_mqtt();
    TEST_ASSERT_EQUAL(PORT_OK,
                      mqtt_feed_overfill_handle("{\"enabled\":true}", strlen("{\"enabled\":true}")));
    TEST_ASSERT_TRUE(feed_config_overfill_enabled_get());
    TEST_ASSERT_EQUAL_UINT8(50u, feed_config_overfill_threshold_g_get());
}

void test_mqtt_feed_overfill_handle_partial_threshold(void)
{
    setup_feed_overfill_mqtt();
    TEST_ASSERT_EQUAL(PORT_OK,
                      mqtt_feed_overfill_handle("{\"threshold_g\":40}",
                                                strlen("{\"threshold_g\":40}")));
    TEST_ASSERT_FALSE(feed_config_overfill_enabled_get());
    TEST_ASSERT_EQUAL_UINT8(40u, feed_config_overfill_threshold_g_get());
}

void test_mqtt_feed_overfill_handle_rejects_invalid_threshold(void)
{
    setup_feed_overfill_mqtt();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      mqtt_feed_overfill_handle("{\"threshold_g\":20}",
                                                strlen("{\"threshold_g\":20}")));
}

void test_mqtt_feed_overfill_format_snapshot(void)
{
    char buf[64];

    fake_config_port_reset();
    TEST_ASSERT_TRUE(feed_config_overfill_enabled_set(true));
    TEST_ASSERT_TRUE(feed_config_overfill_threshold_g_set(75u));
    TEST_ASSERT_TRUE(mqtt_feed_overfill_format_snapshot(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("{\"enabled\":true,\"threshold_g\":75}", buf);
}
