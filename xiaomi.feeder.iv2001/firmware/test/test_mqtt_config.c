/* Tests: spec/30-processes/mqtt-protocol.md § Config snapshot */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "fake_time_port.h"
#include "mqtt_config.h"
#include "mqtt_outbox.h"
#include "mqtt_timezone.h"
#include "time_sync.h"
#include "tz_rule.h"

#define TEST_DEVICE_ID "ddeeff"
#define BUCHAREST_POSIX "EET-2EEST,M3.5.0/3,M10.5.0/4"

static void drain_config_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void setup_mqtt_config(void)
{
    fake_config_port_reset();
    fake_mqtt_port_reset();
    fake_time_port_reset();
    mqtt_outbox_reset();
    mqtt_config_test_reset();
    mqtt_timezone_test_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_config_set_device_id(TEST_DEVICE_ID);
    mqtt_timezone_set_device_id(TEST_DEVICE_ID);
}

void test_mqtt_config_format_snapshot_defaults(void)
{
    char payload[256];

    fake_config_port_reset();
    time_sync_test_reset();
    TEST_ASSERT_TRUE(mqtt_config_format_snapshot(payload, sizeof(payload)));
    TEST_ASSERT_NOT_NULL(strstr(payload, "\"tz_rule\":\"UTC0\""));
    TEST_ASSERT_NOT_NULL(strstr(payload, "\"time_synced\":false"));
    TEST_ASSERT_NOT_NULL(strstr(payload, "\"utc_epoch\":0"));
}

void test_mqtt_config_handle_sets_tz_rule(void)
{
    const fake_mqtt_port_state_t *mqtt;
    char loaded[TZ_RULE_POSIX_MAX];
    const config_port_t *cfg = fake_config_port_get();
    static const char json[] = "{\"tz_rule\":\"EET-2EEST,M3.5.0/3,M10.5.0/4\"}";

    setup_mqtt_config();
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_config_handle(json, strlen(json)));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_load_posix(cfg, loaded, sizeof(loaded)));
    TEST_ASSERT_EQUAL_STRING(BUCHAREST_POSIX, loaded);

    drain_config_outbox();
    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, BUCHAREST_POSIX));
}

void test_mqtt_config_handle_rejects_unknown_keys(void)
{
    setup_mqtt_config();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      mqtt_config_handle("{\"child_lock\":true}",
                                         strlen("{\"child_lock\":true}")));
}

void test_mqtt_config_handle_sets_tz_label(void)
{
    char label[TZ_RULE_LABEL_MAX];
    const config_port_t *cfg = fake_config_port_get();

    setup_mqtt_config();
    TEST_ASSERT_EQUAL(PORT_OK,
                      mqtt_config_handle("{\"tz_label\":\"Europe/Bucharest\"}",
                                         strlen("{\"tz_label\":\"Europe/Bucharest\"}")));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_label_load(cfg, label, sizeof(label)));
    TEST_ASSERT_EQUAL_STRING("Europe/Bucharest", label);
}

void test_mqtt_config_connect_snapshot_publishes(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_mqtt_config();
    mqtt_config_connect_snapshot();
    drain_config_outbox();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/config", mqtt->last_publish_topic);
}

void test_mqtt_config_handle_rejects_invalid_tz_rule(void)
{
    setup_mqtt_config();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      mqtt_config_handle("{\"tz_rule\":\"480\"}",
                                         strlen("{\"tz_rule\":\"480\"}")));
}

void test_mqtt_config_handle_parses_escaped_tz_label(void)
{
    char label[TZ_RULE_LABEL_MAX];
    const config_port_t *cfg = fake_config_port_get();

    setup_mqtt_config();
    TEST_ASSERT_EQUAL(PORT_OK,
                      mqtt_config_handle("{\"tz_label\":\"Europe/\\\"Test\\\"\"}",
                                         strlen("{\"tz_label\":\"Europe/\\\"Test\\\"\"}")));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_label_load(cfg, label, sizeof(label)));
    TEST_ASSERT_EQUAL_STRING("Europe/\"Test\"", label);
}

void test_mqtt_config_handle_rejects_long_tz_label(void)
{
    static const char payload[] =
        "{\"tz_label\":\"abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMN\"}";

    setup_mqtt_config();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      mqtt_config_handle(payload, strlen(payload)));
}

void test_mqtt_config_handle_clears_tz_label(void)
{
    char label[TZ_RULE_LABEL_MAX];
    const config_port_t *cfg = fake_config_port_get();

    setup_mqtt_config();
    TEST_ASSERT_EQUAL(PORT_OK,
                      mqtt_config_handle("{\"tz_label\":\"Europe/Bucharest\"}",
                                         strlen("{\"tz_label\":\"Europe/Bucharest\"}")));
    TEST_ASSERT_EQUAL(PORT_OK,
                      mqtt_config_handle("{\"tz_label\":\"\"}",
                                         strlen("{\"tz_label\":\"\"}")));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_label_load(cfg, label, sizeof(label)));
    TEST_ASSERT_EQUAL_STRING("", label);
}

void test_mqtt_config_handle_clears_tz_rule_to_utc0(void)
{
    char loaded[TZ_RULE_POSIX_MAX];
    const config_port_t *cfg = fake_config_port_get();

    setup_mqtt_config();
    TEST_ASSERT_EQUAL(PORT_OK,
                      mqtt_config_handle("{\"tz_rule\":\"EET-2EEST,M3.5.0/3,M10.5.0/4\"}",
                                         strlen("{\"tz_rule\":\"EET-2EEST,M3.5.0/3,M10.5.0/4\"}")));
    TEST_ASSERT_EQUAL(PORT_OK,
                      mqtt_config_handle("{\"tz_rule\":\"\"}",
                                         strlen("{\"tz_rule\":\"\"}")));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_load_posix(cfg, loaded, sizeof(loaded)));
    TEST_ASSERT_EQUAL_STRING("UTC0", loaded);
}
