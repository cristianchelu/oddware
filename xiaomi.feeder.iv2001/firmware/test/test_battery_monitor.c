/* Tests: spec/30-processes/battery-monitoring.md § Sample interval */

#include "unity.h"

#include "battery_monitor.h"
#include "fake_adc_port.h"
#include "fake_mqtt_port.h"
#include "fake_power_source_port.h"
#include "fake_time.h"
#include "mqtt_battery.h"
#include "mqtt_battery_voltage.h"
#include "mqtt_outbox.h"
#include "power_source_input.h"
#include "port_err.h"

#define TEST_DEVICE_ID "ddeeff"

static void setup_battery_monitor(bool mains_present)
{
    fake_time_reset();
    fake_power_source_port_reset();
    fake_adc_port_reset();
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    mqtt_battery_test_reset();
    mqtt_battery_voltage_test_reset();
    battery_monitor_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_battery_set_device_id(TEST_DEVICE_ID);
    mqtt_battery_voltage_set_device_id(TEST_DEVICE_ID);

    fake_power_source_port_set_mains_present(mains_present);
    power_source_input_init(fake_power_source_port_get());
}

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

void test_battery_monitor_samples_immediately_on_first_poll(void)
{
    setup_battery_monitor(false);
    fake_adc_port_set_battery_mv(5200u);

    battery_monitor_poll(0u);
    drain_outbox();

    TEST_ASSERT_EQUAL_UINT(2, fake_mqtt_port_state()->publish_calls);
    TEST_ASSERT_EQUAL_STRING("50", fake_mqtt_port_state()->last_publish_payload);
}

void test_battery_monitor_zero_mv_publishes_unknown_soc_and_zero_voltage(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_battery_monitor(false);
    fake_adc_port_set_battery_mv(0u);

    battery_monitor_poll(0u);
    drain_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/battery_voltage", mqtt->prior_publish_topic);
    TEST_ASSERT_EQUAL_STRING("0", mqtt->prior_publish_payload);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/battery", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("unknown", mqtt->last_publish_payload);
}

void test_battery_monitor_battery_interval_is_60s(void)
{
    setup_battery_monitor(false);
    fake_adc_port_set_battery_mv(5200u);

    battery_monitor_poll(0u);
    drain_outbox();

    fake_adc_port_set_battery_mv(4800u);
    battery_monitor_poll(59000u);
    drain_outbox();
    TEST_ASSERT_EQUAL_UINT(2, fake_mqtt_port_state()->publish_calls);

    battery_monitor_poll(60000u);
    drain_outbox();
    TEST_ASSERT_EQUAL_UINT(4, fake_mqtt_port_state()->publish_calls);
    TEST_ASSERT_EQUAL_STRING("25", fake_mqtt_port_state()->last_publish_payload);
}

void test_battery_monitor_mains_interval_is_300s(void)
{
    setup_battery_monitor(true);
    fake_adc_port_set_battery_mv(6000u);

    battery_monitor_poll(0u);
    drain_outbox();

    fake_adc_port_set_battery_mv(5600u);
    battery_monitor_poll(299000u);
    drain_outbox();
    TEST_ASSERT_EQUAL_UINT(2, fake_mqtt_port_state()->publish_calls);

    battery_monitor_poll(300000u);
    drain_outbox();
    TEST_ASSERT_EQUAL_UINT(4, fake_mqtt_port_state()->publish_calls);
    TEST_ASSERT_EQUAL_STRING("75", fake_mqtt_port_state()->last_publish_payload);
}

void test_battery_monitor_skips_on_adc_busy(void)
{
    setup_battery_monitor(false);
    fake_adc_port_set_battery_err(PORT_ERR_BUSY);

    TEST_ASSERT_FALSE(battery_monitor_poll(0u));
    drain_outbox();

    TEST_ASSERT_EQUAL_UINT(0, fake_mqtt_port_state()->publish_calls);

    fake_adc_port_set_battery_err(PORT_OK);
    fake_adc_port_set_battery_mv(5200u);
    TEST_ASSERT_TRUE(battery_monitor_poll(1000u));
    drain_outbox();

    TEST_ASSERT_EQUAL_UINT(2, fake_mqtt_port_state()->publish_calls);
}

void test_battery_monitor_skips_on_adc_error(void)
{
    setup_battery_monitor(false);
    fake_adc_port_set_battery_err(PORT_ERR_IO);

    TEST_ASSERT_FALSE(battery_monitor_poll(0u));
    drain_outbox();

    TEST_ASSERT_EQUAL_UINT(0, fake_mqtt_port_state()->publish_calls);
}

void test_battery_monitor_force_sample_bypasses_interval(void)
{
    setup_battery_monitor(true);
    fake_adc_port_set_battery_mv(6000u);

    battery_monitor_poll(0u);
    drain_outbox();

    fake_adc_port_set_battery_mv(5200u);
    battery_monitor_force_sample();
    battery_monitor_poll(1000u);
    drain_outbox();

    TEST_ASSERT_EQUAL_UINT(4, fake_mqtt_port_state()->publish_calls);
    TEST_ASSERT_EQUAL_STRING("50", fake_mqtt_port_state()->last_publish_payload);
}

void test_battery_monitor_skips_when_power_source_invalid(void)
{
    fake_time_reset();
    fake_power_source_port_reset();
    fake_adc_port_reset();
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    mqtt_battery_test_reset();
    mqtt_battery_voltage_test_reset();
    battery_monitor_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_battery_set_device_id(TEST_DEVICE_ID);
    mqtt_battery_voltage_set_device_id(TEST_DEVICE_ID);

    fake_power_source_port_set_read_err(PORT_ERR_IO);
    power_source_input_init(fake_power_source_port_get());
    fake_adc_port_set_battery_mv(5200u);

    battery_monitor_poll(0u);
    drain_outbox();

    TEST_ASSERT_EQUAL_UINT(0, fake_mqtt_port_state()->publish_calls);
}
