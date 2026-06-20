/* Tests: spec/30-processes/mqtt-protocol.md § Device timezone */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "fake_time_port.h"
#include "mqtt_outbox.h"
#include "mqtt_timezone.h"
#include "tz_rule.h"

#define TEST_DEVICE_ID "ddeeff"
#define BUCHAREST_POSIX "EET-2EEST,M3.5.0/3,M10.5.0/4"

static void drain_timezone_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void setup_mqtt_timezone(void)
{
    fake_config_port_reset();
    fake_mqtt_port_reset();
    fake_time_port_reset();
    mqtt_outbox_reset();
    mqtt_timezone_test_reset();
    tz_rule_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_timezone_set_device_id(TEST_DEVICE_ID);
}

void test_mqtt_timezone_publish_defaults_to_utc0(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_timezone();
    mqtt_timezone_publish_snapshot();
    drain_timezone_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/timezone", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("UTC0", mqtt->last_publish_payload);
}

void test_mqtt_timezone_publish_prefers_label(void)
{
    const fake_mqtt_port_state_t *mqtt;
    const config_port_t *cfg = fake_config_port_get();

    setup_mqtt_timezone();
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg, BUCHAREST_POSIX));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_label_save(cfg, "Europe/Bucharest"));

    mqtt_timezone_publish_snapshot();
    drain_timezone_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_STRING("Europe/Bucharest", mqtt->last_publish_payload);
}

void test_mqtt_timezone_publish_falls_back_to_posix(void)
{
    const fake_mqtt_port_state_t *mqtt;
    const config_port_t *cfg = fake_config_port_get();

    setup_mqtt_timezone();
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg, BUCHAREST_POSIX));

    mqtt_timezone_publish_snapshot();
    drain_timezone_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_STRING(BUCHAREST_POSIX, mqtt->last_publish_payload);
}

void test_mqtt_timezone_publish_dedupes_unchanged(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_timezone();
    mqtt_timezone_publish_snapshot();
    mqtt_timezone_publish_snapshot();
    drain_timezone_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
}

void test_mqtt_timezone_connect_snapshot_republishes(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_timezone();
    mqtt_timezone_publish_snapshot();
    drain_timezone_outbox();

    mqtt_timezone_connect_snapshot();
    drain_timezone_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("UTC0", mqtt->last_publish_payload);
}
