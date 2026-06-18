/* Tests: spec/30-processes/mqtt-protocol.md § Battery pack voltage */

#include <string.h>

#include "unity.h"

#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "mqtt_battery_voltage.h"
#include "mqtt_outbox.h"

#define TEST_DEVICE_ID "ddeeff"

static void drain_voltage_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void setup_mqtt_battery_voltage(void)
{
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_battery_voltage_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_battery_voltage_set_device_id(TEST_DEVICE_ID);
}

void test_mqtt_battery_voltage_sync_publishes_plain_mv(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery_voltage();
    mqtt_battery_voltage_sync(5200u, false);
    drain_voltage_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/battery_voltage", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("5200", mqtt->last_publish_payload);
}

void test_mqtt_battery_voltage_sync_publishes_zero_mv(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery_voltage();
    mqtt_battery_voltage_sync(0u, false);
    drain_voltage_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_STRING("0", mqtt->last_publish_payload);
}

void test_mqtt_battery_voltage_sync_edge_detects_small_change(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery_voltage();
    mqtt_battery_voltage_sync(5200u, false);
    drain_voltage_outbox();
    mqtt_battery_voltage_sync(5205u, false);
    drain_voltage_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
}

void test_mqtt_battery_voltage_sync_publishes_on_10mv_change(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery_voltage();
    mqtt_battery_voltage_sync(5200u, false);
    drain_voltage_outbox();
    mqtt_battery_voltage_sync(5210u, false);
    drain_voltage_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("5210", mqtt->last_publish_payload);
}

void test_mqtt_battery_voltage_on_mqtt_connected_snapshots_value(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery_voltage();
    mqtt_battery_voltage_sync(4800u, false);
    drain_voltage_outbox();

    fake_mqtt_port_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_outbox_reset();

    mqtt_battery_voltage_on_mqtt_connected();
    drain_voltage_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("4800", mqtt->last_publish_payload);
}

void test_mqtt_battery_voltage_on_mqtt_connected_skips_when_unknown(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery_voltage();
    mqtt_battery_voltage_on_mqtt_connected();
    drain_voltage_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->publish_calls);
}

void test_mqtt_battery_voltage_sync_force_republishes_unchanged(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_battery_voltage();
    mqtt_battery_voltage_sync(6000u, false);
    drain_voltage_outbox();
    mqtt_battery_voltage_sync(6000u, true);
    drain_voltage_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
}
