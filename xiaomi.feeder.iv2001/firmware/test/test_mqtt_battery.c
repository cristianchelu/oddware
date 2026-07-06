/* Tests: spec/30-processes/mqtt-protocol.md § Battery */

#include <string.h>

#include "unity.h"

#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "mqtt_battery.h"
#include "mqtt_outbox.h"

#define TEST_DEVICE_ID "ddeeff"

static void drain_battery_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void setup_mqtt_battery(void)
{
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_battery_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_battery_set_device_id(TEST_DEVICE_ID);
}

void test_mqtt_battery_sync_publishes_plain_integer(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_sync(true, 75u, false);
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/battery", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("75", mqtt->last_publish_payload);
}

void test_mqtt_battery_sync_cached_before_topic_configured(void)
{
    const fake_mqtt_port_state_t *mqtt;
    char wire[8];

    setup_mqtt_battery();
    mqtt_battery_test_reset();
    mqtt_battery_sync(true, 72u, false);

    TEST_ASSERT_TRUE(mqtt_battery_format_wire(wire, sizeof(wire)));
    TEST_ASSERT_EQUAL_STRING("72", wire);
    TEST_ASSERT_EQUAL_UINT(0, fake_mqtt_port_state()->publish_calls);

    mqtt_battery_set_device_id(TEST_DEVICE_ID);
    mqtt_battery_on_mqtt_connected();
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("72", mqtt->last_publish_payload);
}

void test_mqtt_battery_sync_unknown_publishes_unknown_payload(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_sync(false, 0u, false);
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/battery", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("unknown", mqtt->last_publish_payload);
}

void test_mqtt_battery_sync_known_to_unknown_transition(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_sync(true, 75u, false);
    drain_battery_outbox();
    mqtt_battery_sync(false, 0u, false);
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("unknown", mqtt->last_publish_payload);
}

void test_mqtt_battery_sync_edge_detects_unchanged_value(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_sync(true, 75u, false);
    drain_battery_outbox();
    mqtt_battery_sync(true, 75u, false);
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
}

void test_mqtt_battery_sync_publishes_on_one_point_change(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_sync(true, 75u, false);
    drain_battery_outbox();
    mqtt_battery_sync(true, 76u, false);
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("76", mqtt->last_publish_payload);
}

void test_mqtt_battery_on_mqtt_connected_snapshots_known_value(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_sync(true, 42u, false);
    drain_battery_outbox();

    fake_mqtt_port_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_outbox_reset();

    mqtt_battery_on_mqtt_connected();
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("42", mqtt->last_publish_payload);
}

void test_mqtt_battery_on_mqtt_connected_snapshots_unknown_value(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_sync(false, 0u, false);
    drain_battery_outbox();

    fake_mqtt_port_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_outbox_reset();

    mqtt_battery_on_mqtt_connected();
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("unknown", mqtt->last_publish_payload);
}

void test_mqtt_battery_on_mqtt_connected_skips_when_unknown(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_on_mqtt_connected();
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->publish_calls);
}

void test_mqtt_battery_on_outbox_reset_allows_republish(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_sync(true, 50u, false);
    drain_battery_outbox();

    mqtt_battery_on_outbox_reset();
    mqtt_battery_sync(true, 50u, false);
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
}

void test_mqtt_battery_sync_force_republishes_unchanged(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery();
    mqtt_battery_sync(true, 80u, false);
    drain_battery_outbox();
    mqtt_battery_sync(true, 80u, true);
    drain_battery_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
}
