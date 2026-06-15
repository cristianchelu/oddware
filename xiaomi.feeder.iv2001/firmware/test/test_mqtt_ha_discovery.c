/* Tests: spec/30-processes/mqtt-protocol.md § Home Assistant validation slice */

#include <string.h>

#include "unity.h"

#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "mqtt_ha_discovery.h"
#include "mqtt_outbox.h"

#define TEST_DEVICE_ID "ddeeff"

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

void test_mqtt_ha_format_dispense_button_config_json(void)
{
    char buf[512];
    int written;

    written = mqtt_ha_format_dispense_button_config(buf, sizeof(buf), TEST_DEVICE_ID);
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"Dispense\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"unique_id\":\"petfeeder_ddeeff_dispense\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"command_topic\":\"petfeeder/ddeeff/cmd/dispense\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"availability_topic\":\"petfeeder/ddeeff/connection\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"payload_available\":\"online\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"payload_not_available\":\"offline\""));
    TEST_ASSERT_NULL(strstr(buf, "value_template"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"manufacturer\":\"Oddware\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"model\":\"IV2001 Pet Feeder\""));
}

void test_mqtt_ha_discovery_schedule_enqueues_one_item(void)
{
    mqtt_outbox_reset();
    mqtt_ha_discovery_schedule(TEST_DEVICE_ID);
    TEST_ASSERT_EQUAL_UINT(1, mqtt_outbox_pending());
}

void test_mqtt_ha_discovery_schedule_drain_publishes_retained(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    fake_mqtt_port_get()->connect(NULL);

    mqtt_ha_discovery_schedule(TEST_DEVICE_ID);
    drain_all_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("homeassistant/button/petfeeder_ddeeff/dispense/config",
                             mqtt->last_publish_topic);
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"command_topic\""));
}
