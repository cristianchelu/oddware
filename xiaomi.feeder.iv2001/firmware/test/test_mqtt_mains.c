/* Tests: spec/30-processes/mqtt-protocol.md § Mains */

#include <string.h>

#include "unity.h"

#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "mqtt_mains.h"
#include "mqtt_outbox.h"

#define TEST_DEVICE_ID "ddeeff"

static void drain_mains_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void setup_mqtt_mains(void)
{
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_mains_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_mains_set_device_id(TEST_DEVICE_ID);
}

void test_mqtt_mains_sync_publishes_on(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_mains();
    mqtt_mains_sync(true);
    drain_mains_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/mains", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("ON", mqtt->last_publish_payload);
}

void test_mqtt_mains_sync_publishes_off(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_mains();
    mqtt_mains_sync(false);
    drain_mains_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_STRING("OFF", mqtt->last_publish_payload);
}

void test_mqtt_mains_sync_edge_detects_unchanged_value(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_mains();
    mqtt_mains_sync(true);
    drain_mains_outbox();
    mqtt_mains_sync(true);
    drain_mains_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
}

void test_mqtt_mains_sync_publishes_on_change(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_mains();
    mqtt_mains_sync(true);
    drain_mains_outbox();
    mqtt_mains_sync(false);
    drain_mains_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("OFF", mqtt->last_publish_payload);
}

void test_mqtt_mains_connect_snapshot_publishes_once(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_mains();
    mqtt_mains_connect_snapshot(true);
    drain_mains_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("ON", mqtt->last_publish_payload);

    mqtt_mains_connect_snapshot(true);
    drain_mains_outbox();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
}

void test_mqtt_mains_on_mqtt_connected_snapshots_known_value(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_mains();
    mqtt_mains_sync(false);
    drain_mains_outbox();

    fake_mqtt_port_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_outbox_reset();

    mqtt_mains_on_mqtt_connected();
    drain_mains_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("OFF", mqtt->last_publish_payload);
}

void test_mqtt_mains_on_mqtt_connected_skips_when_unknown(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_mains();
    mqtt_mains_on_mqtt_connected();
    drain_mains_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->publish_calls);
}

void test_mqtt_mains_on_outbox_reset_allows_republish(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_mains();
    mqtt_mains_sync(true);
    drain_mains_outbox();

    mqtt_mains_on_outbox_reset();
    mqtt_mains_sync(true);
    drain_mains_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
}
