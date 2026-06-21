/* Tests: spec/30-processes/mqtt-protocol.md § Publish path */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "mqtt_outbox.h"

#define TEST_TOPIC   "petfeeder/ddeeff/ota/status"
#define TEST_PAYLOAD "{\"state\": \"idle\", \"pct\": 0, \"error\": \"\"}"

static void drain_all_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void outbox_test_reset(void)
{
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
}

void test_mqtt_outbox_enqueue_empty_payload(void)
{
    outbox_test_reset();
    TEST_ASSERT_TRUE(mqtt_outbox_enqueue(TEST_TOPIC, "", 0u, 1, true));
    TEST_ASSERT_EQUAL_UINT(1, mqtt_outbox_pending());
}

void test_mqtt_outbox_drain_publishes_empty_payload(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    outbox_test_reset();
    fake_mqtt_port_get()->connect(NULL);

    TEST_ASSERT_TRUE(mqtt_outbox_enqueue(TEST_TOPIC, "", 0u, 1, true));
    TEST_ASSERT_TRUE(mqtt_outbox_drain_one(fake_mqtt_port_get()));

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("", mqtt->last_publish_payload);
}

void test_mqtt_outbox_enqueue_copies_payload(void)
{
    outbox_test_reset();
    TEST_ASSERT_TRUE(mqtt_outbox_enqueue(TEST_TOPIC,
                                         TEST_PAYLOAD,
                                         strlen(TEST_PAYLOAD),
                                         1,
                                         true));
    TEST_ASSERT_EQUAL_UINT(1, mqtt_outbox_pending());
}

void test_mqtt_outbox_drain_publishes_via_fake_port(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    outbox_test_reset();
    fake_mqtt_port_get()->connect(NULL);

    TEST_ASSERT_TRUE(mqtt_outbox_enqueue(TEST_TOPIC,
                                         TEST_PAYLOAD,
                                         strlen(TEST_PAYLOAD),
                                         1,
                                         true));
    TEST_ASSERT_TRUE(mqtt_outbox_drain_one(fake_mqtt_port_get()));

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING(TEST_TOPIC, mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING(TEST_PAYLOAD, mqtt->last_publish_payload);
    TEST_ASSERT_EQUAL_UINT(0, mqtt_outbox_pending());
}

void test_mqtt_outbox_rate_interval_respected(void)
{
    const fake_mqtt_port_state_t *mqtt;
    const char *payload2 = "{\"state\": \"downloading\", \"pct\": 1, \"error\": \"\"}";

    fake_time_reset();
    fake_mqtt_port_reset();
    outbox_test_reset();
    fake_mqtt_port_get()->connect(NULL);

    TEST_ASSERT_TRUE(mqtt_outbox_enqueue(TEST_TOPIC,
                                         TEST_PAYLOAD,
                                         strlen(TEST_PAYLOAD),
                                         1,
                                         true));
    TEST_ASSERT_TRUE(mqtt_outbox_enqueue(TEST_TOPIC,
                                         payload2,
                                         strlen(payload2),
                                         1,
                                         true));
    TEST_ASSERT_TRUE(mqtt_outbox_drain_one(fake_mqtt_port_get()));
    TEST_ASSERT_FALSE(mqtt_outbox_drain_one(fake_mqtt_port_get()));
    TEST_ASSERT_EQUAL_UINT(1, mqtt_outbox_pending());

    fake_time_advance_ms(99u);
    TEST_ASSERT_FALSE(mqtt_outbox_drain_one(fake_mqtt_port_get()));

    fake_time_advance_ms(1u);
    TEST_ASSERT_TRUE(mqtt_outbox_drain_one(fake_mqtt_port_get()));

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING(payload2, mqtt->last_publish_payload);
    TEST_ASSERT_EQUAL_UINT(0, mqtt_outbox_pending());
}

void test_mqtt_outbox_full_ring_drops(void)
{
    char topic[32];
    unsigned i;

    outbox_test_reset();

    for (i = 0; i < 20u; i++) {
        snprintf(topic, sizeof(topic), "petfeeder/ddeeff/t%u", i);
        TEST_ASSERT_TRUE(mqtt_outbox_enqueue(topic, "x", 1, 0, false));
    }

    TEST_ASSERT_FALSE(mqtt_outbox_enqueue(TEST_TOPIC, "y", 1, 0, false));
    TEST_ASSERT_EQUAL_UINT(20, mqtt_outbox_pending());
}

void test_mqtt_outbox_not_accepting_drops_silently(void)
{
    outbox_test_reset();
    mqtt_outbox_set_accepting(false);

    TEST_ASSERT_FALSE(mqtt_outbox_enqueue(TEST_TOPIC, "x", 1, 0, false));
    TEST_ASSERT_EQUAL_UINT(0, mqtt_outbox_pending());

    mqtt_outbox_set_accepting(true);
    TEST_ASSERT_TRUE(mqtt_outbox_enqueue(TEST_TOPIC, "x", 1, 0, false));
    TEST_ASSERT_EQUAL_UINT(1, mqtt_outbox_pending());
}

void test_mqtt_outbox_reset_clears(void)
{
    outbox_test_reset();
    TEST_ASSERT_TRUE(mqtt_outbox_enqueue(TEST_TOPIC,
                                         TEST_PAYLOAD,
                                         strlen(TEST_PAYLOAD),
                                         1,
                                         true));
    outbox_test_reset();
    TEST_ASSERT_EQUAL_UINT(0, mqtt_outbox_pending());

    fake_time_reset();
    fake_mqtt_port_reset();
    fake_mqtt_port_get()->connect(NULL);
    TEST_ASSERT_TRUE(mqtt_outbox_enqueue(TEST_TOPIC,
                                         TEST_PAYLOAD,
                                         strlen(TEST_PAYLOAD),
                                         1,
                                         true));
    drain_all_outbox();
    TEST_ASSERT_EQUAL_UINT(1, fake_mqtt_port_state()->publish_calls);
}
