/* Tests: spec/30-processes/mqtt-protocol.md § Bowl weight */

#include <string.h>

#include "unity.h"

#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "mqtt_bowl_weight.h"
#include "mqtt_outbox.h"

#define TEST_DEVICE_ID "ddeeff"

static void drain_bowl_weight_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void setup_mqtt_bowl_weight(void)
{
    fake_time_reset();
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_bowl_weight_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_bowl_weight_set_device_id(TEST_DEVICE_ID);
}

void test_mqtt_bowl_weight_sync_publishes_plain_grams(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 42, false);
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/bowl_weight", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("42", mqtt->last_publish_payload);
}

void test_mqtt_bowl_weight_sync_publishes_empty_when_unknown(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_sync(BOWL_GRAMS_UNKNOWN, 0, false);
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("", mqtt->last_publish_payload);
}

void test_mqtt_bowl_weight_sync_edge_detects_small_change(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 42, false);
    drain_bowl_weight_outbox();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 43, false);
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
}

void test_mqtt_bowl_weight_sync_publishes_on_two_gram_change(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 42, false);
    drain_bowl_weight_outbox();
    fake_time_advance_ms(MQTT_BOWL_WEIGHT_COALESCE_MS);
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 44, false);
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("44", mqtt->last_publish_payload);
}

void test_mqtt_bowl_weight_sync_known_unknown_transition(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 42, false);
    drain_bowl_weight_outbox();
    mqtt_bowl_weight_sync(BOWL_GRAMS_UNKNOWN, 0, false);
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("", mqtt->last_publish_payload);
}

void test_mqtt_bowl_weight_coalesce_within_two_seconds(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 42, false);
    drain_bowl_weight_outbox();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 50, false);
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);

    fake_time_advance_ms(MQTT_BOWL_WEIGHT_COALESCE_MS);
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 50, false);
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("50", mqtt->last_publish_payload);
}

void test_mqtt_bowl_weight_force_bypasses_deadband(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 42, false);
    drain_bowl_weight_outbox();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 43, true);
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
}

void test_mqtt_bowl_weight_on_mqtt_connected_snapshots_known_value(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 42, false);
    drain_bowl_weight_outbox();

    fake_mqtt_port_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_outbox_reset();

    mqtt_bowl_weight_on_mqtt_connected();
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("42", mqtt->last_publish_payload);
}

void test_mqtt_bowl_weight_on_mqtt_connected_skips_when_unknown(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_on_mqtt_connected();
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->publish_calls);
}

void test_mqtt_bowl_weight_on_outbox_reset_allows_republish(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_bowl_weight();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 42, false);
    drain_bowl_weight_outbox();

    mqtt_bowl_weight_on_outbox_reset();
    mqtt_bowl_weight_sync(BOWL_GRAMS_KNOWN, 42, false);
    drain_bowl_weight_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
}
