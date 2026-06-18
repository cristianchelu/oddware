/* Tests: spec/30-processes/mqtt-protocol.md § Dispense event */

#include <string.h>

#include "unity.h"

#include "dispense.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "mqtt_dispense_event.h"
#include "mqtt_outbox.h"

#define TEST_DEVICE_ID "abc123"

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

void test_mqtt_dispense_event_publish_measured_json(void)
{
    dispense_completion_t completion;
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    fake_mqtt_port_get()->connect(NULL);
    mqtt_dispense_event_set_device_id(TEST_DEVICE_ID);

    memset(&completion, 0, sizeof(completion));
    completion.grams = 28;
    completion.grams_estimated = false;
    completion.target_g = 30;
    completion.outcome = DISPENSE_OUTCOME_SUCCESS;
    completion.source = DISPENSE_SOURCE_MQTT;
    completion.mode = DISPENSE_MODE_OPEN_LOOP;
    completion.batch_count = 1u;

    TEST_ASSERT_TRUE(mqtt_dispense_event_publish(&completion));
    drain_all_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_STRING("petfeeder/abc123/dispense/event", mqtt->last_publish_topic);
    TEST_ASSERT_FALSE(mqtt->last_publish_retain);
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"event_type\":\"success\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"grams\":28"));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"grams_estimated\":false"));
    TEST_ASSERT_NULL(strstr(mqtt->last_publish_payload, "\"outcome\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"source\":\"mqtt\""));
}

void test_mqtt_dispense_event_publish_estimated_json(void)
{
    dispense_completion_t completion;
    const fake_mqtt_port_state_t *mqtt;

    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    fake_mqtt_port_get()->connect(NULL);
    mqtt_dispense_event_set_device_id(TEST_DEVICE_ID);

    memset(&completion, 0, sizeof(completion));
    completion.grams = 10;
    completion.grams_estimated = true;
    completion.target_g = 10;
    completion.outcome = DISPENSE_OUTCOME_SUCCESS;
    completion.source = DISPENSE_SOURCE_BUTTON;
    completion.mode = DISPENSE_MODE_OPEN_LOOP;
    completion.batch_count = 1u;

    TEST_ASSERT_TRUE(mqtt_dispense_event_publish(&completion));
    drain_all_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"event_type\":\"success\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"grams_estimated\":true"));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"source\":\"button\""));
}

void test_mqtt_dispense_event_drops_when_offline(void)
{
    dispense_completion_t completion;

    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    mqtt_dispense_event_set_device_id(TEST_DEVICE_ID);

    memset(&completion, 0, sizeof(completion));
    completion.grams = 10;
    completion.target_g = 10;
    completion.outcome = DISPENSE_OUTCOME_SUCCESS;
    completion.source = DISPENSE_SOURCE_UART;
    completion.mode = DISPENSE_MODE_OPEN_LOOP;
    completion.batch_count = 1u;

    TEST_ASSERT_FALSE(mqtt_dispense_event_publish(&completion));
    TEST_ASSERT_EQUAL_UINT(0, mqtt_outbox_pending());
}

void test_mqtt_dispense_event_stuck_outcome(void)
{
    dispense_completion_t completion;
    const fake_mqtt_port_state_t *mqtt;

    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    fake_mqtt_port_get()->connect(NULL);
    mqtt_dispense_event_set_device_id(TEST_DEVICE_ID);

    memset(&completion, 0, sizeof(completion));
    completion.grams = 5;
    completion.grams_estimated = false;
    completion.target_g = 10;
    completion.outcome = DISPENSE_OUTCOME_STUCK;
    completion.source = DISPENSE_SOURCE_UART;
    completion.mode = DISPENSE_MODE_OPEN_LOOP;
    completion.batch_count = 1u;

    TEST_ASSERT_TRUE(mqtt_dispense_event_publish(&completion));
    drain_all_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"event_type\":\"stuck\""));
    TEST_ASSERT_NULL(strstr(mqtt->last_publish_payload, "\"outcome\""));
}
