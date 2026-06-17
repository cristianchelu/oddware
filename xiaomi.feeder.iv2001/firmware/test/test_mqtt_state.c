/* Tests: spec/30-processes/mqtt-protocol.md § Device condition */

#include <string.h>

#include "unity.h"

#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "mqtt_outbox.h"
#include "mqtt_state.h"

#define TEST_DEVICE_ID "ddeeff"

static void drain_state_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void setup_mqtt_state(void)
{
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_state_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_state_set_device_id(TEST_DEVICE_ID);
}

void test_mqtt_state_sync_publishes_bowl_error_json(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_state();
    mqtt_state_sync(true);
    drain_state_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/state", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("{\"bowl_error\": true}", mqtt->last_publish_payload);
}

void test_mqtt_state_sync_edge_detects_unchanged_value(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_state();
    mqtt_state_sync(true);
    drain_state_outbox();
    mqtt_state_sync(true);
    drain_state_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
}

void test_mqtt_state_sync_publishes_on_change(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_state();
    mqtt_state_sync(true);
    drain_state_outbox();
    mqtt_state_sync(false);
    drain_state_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("{\"bowl_error\": false}", mqtt->last_publish_payload);
}

void test_mqtt_state_on_mqtt_connected_snapshots_known_value(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_state();
    mqtt_state_sync(false);
    drain_state_outbox();

    fake_mqtt_port_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_outbox_reset();

    mqtt_state_on_mqtt_connected();
    drain_state_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("{\"bowl_error\": false}", mqtt->last_publish_payload);
}

void test_mqtt_state_on_outbox_reset_allows_republish(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_state();
    mqtt_state_sync(true);
    drain_state_outbox();

    mqtt_state_on_outbox_reset();
    mqtt_state_sync(true);
    drain_state_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
}

void test_mqtt_state_on_mqtt_connected_skips_when_unknown(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_state();
    mqtt_state_on_mqtt_connected();
    drain_state_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->publish_calls);
}

void test_mqtt_state_sync_does_not_spam_while_publish_pending(void)
{
    setup_mqtt_state();
    mqtt_state_sync(false);
    TEST_ASSERT_EQUAL_UINT(1, mqtt_outbox_pending());

    mqtt_state_sync(false);
    TEST_ASSERT_EQUAL_UINT(1, mqtt_outbox_pending());
}
