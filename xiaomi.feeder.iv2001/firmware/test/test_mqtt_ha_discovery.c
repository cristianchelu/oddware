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
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"manufacturer\":\"" MQTT_HA_MANUFACTURER "\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"model\":\"" MQTT_HA_MODEL "\""));
}

void test_mqtt_ha_format_bowl_error_config_json(void)
{
    char buf[512];
    int written;

    written = mqtt_ha_format_bowl_error_config(buf, sizeof(buf), TEST_DEVICE_ID);
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"Bowl error\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_topic\":\"petfeeder/ddeeff/state\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"value_template\":\"{{ value_json.bowl_error }}\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"payload_on\":true"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"payload_off\":false"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"device_class\":\"problem\""));
}

void test_mqtt_ha_format_bowl_weight_config_json(void)
{
    char buf[768];
    int written;

    written = mqtt_ha_format_bowl_weight_config(buf, sizeof(buf), TEST_DEVICE_ID);
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"Bowl weight\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_topic\":\"petfeeder/ddeeff/bowl_weight\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"unit_of_measurement\":\"g\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"device_class\":\"weight\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_class\":\"measurement\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"availability\":["));
    TEST_ASSERT_NOT_NULL(strstr(buf, "value_json.bowl_error == false"));
}

void test_mqtt_ha_format_battery_config_json(void)
{
    char buf[768];
    int written;

    written = mqtt_ha_format_battery_config(buf, sizeof(buf), TEST_DEVICE_ID);
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"Battery\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_topic\":\"petfeeder/ddeeff/battery\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"unit_of_measurement\":\"%\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"device_class\":\"battery\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_class\":\"measurement\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"availability\":["));
    TEST_ASSERT_NOT_NULL(strstr(buf, "{{ 'true' if value != 'unknown' else 'false' }}"));
    TEST_ASSERT_NULL(strstr(buf, "\"value_template\":\"{{ value_json"));
}

void test_mqtt_ha_format_battery_voltage_config_json(void)
{
    char buf[768];
    int written;

    written = mqtt_ha_format_battery_voltage_config(buf, sizeof(buf), TEST_DEVICE_ID);
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"Battery pack voltage\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_topic\":\"petfeeder/ddeeff/battery_voltage\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"unit_of_measurement\":\"mV\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"device_class\":\"voltage\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_class\":\"measurement\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"enabled_by_default\":false"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"availability_topic\":\"petfeeder/ddeeff/connection\""));
}

void test_mqtt_ha_format_mains_config_json(void)
{
    char buf[768];
    int written;

    written = mqtt_ha_format_mains_config(buf, sizeof(buf), TEST_DEVICE_ID);
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"Mains connected\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_topic\":\"petfeeder/ddeeff/mains\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"payload_on\":\"ON\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"payload_off\":\"OFF\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"device_class\":\"power\""));
}

void test_mqtt_ha_format_hopper_level_config_json(void)
{
    char buf[768];
    int written;

    written = mqtt_ha_format_hopper_level_config(buf, sizeof(buf), TEST_DEVICE_ID);
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"Hopper level\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_topic\":\"petfeeder/ddeeff/hopper\""));
    TEST_ASSERT_NULL(strstr(buf, "value_template"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"device_class\":\"enum\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"options\":[\"normal\",\"low\",\"empty\"]"));
}

void test_mqtt_ha_format_device_timezone_config_json(void)
{
    char buf[768];
    int written;

    written = mqtt_ha_format_device_timezone_config(buf, sizeof(buf), TEST_DEVICE_ID);
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"Device timezone\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_topic\":\"petfeeder/ddeeff/timezone\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"unique_id\":\"petfeeder_ddeeff_device_timezone\""));
    TEST_ASSERT_NULL(strstr(buf, "device_class"));
    TEST_ASSERT_NULL(strstr(buf, "value_template"));
}

void test_mqtt_ha_format_dispense_completed_config_json(void)
{
    char buf[768];
    int written;

    written = mqtt_ha_format_dispense_completed_config(buf, sizeof(buf), TEST_DEVICE_ID);
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"name\":\"Dispense completed\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state_topic\":\"petfeeder/ddeeff/dispense/event\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"event_types\":[\"success\",\"underfill\",\"stuck\",\"empty_hopper\",\"aborted\"]"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"unique_id\":\"petfeeder_ddeeff_dispense_completed\""));
}

void test_mqtt_ha_discovery_schedule_enqueues_one_item(void)
{
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    mqtt_ha_discovery_schedule(TEST_DEVICE_ID);
    TEST_ASSERT_EQUAL_UINT(9, mqtt_outbox_pending());
}

void test_mqtt_ha_discovery_schedule_drain_publishes_retained(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    fake_mqtt_port_get()->connect(NULL);

    mqtt_ha_discovery_schedule(TEST_DEVICE_ID);
    drain_all_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(9, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("homeassistant/event/petfeeder_ddeeff/dispense_completed/config",
                             mqtt->last_publish_topic);
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"event_types\":[\"success\",\"underfill\",\"stuck\",\"empty_hopper\",\"aborted\"]"));
}
