/* Tests: spec/30-processes/mqtt-protocol.md § Schedule */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time_port.h"
#include "mqtt_client_test.h"
#include "mqtt_route.h"
#include "mqtt_schedule.h"
#include "schedule.h"
#include "schedule_test_epochs.h"
#include "time_sync.h"
#include "tz_rule.h"

#define TEST_DEVICE_ID "ddeeff"

static void setup_schedule_mqtt(int64_t epoch)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    fake_time_port_reset();
    fake_mqtt_port_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    schedule_test_reset();
    mqtt_schedule_test_reset();
    mqtt_client_test_reset();

    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg, "UTC0"));
    tz_rule_init();

    fake_time_port_set_epoch(epoch);
    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, epoch);
    time_sync_poll(1000u);

    schedule_init();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_client_test_set_device_id(TEST_DEVICE_ID);
}

static const char *mqtt_find_schedule_state_payload(const fake_mqtt_port_state_t *mqtt)
{
    if (mqtt == NULL) {
        return NULL;
    }

    if (strstr(mqtt->last_publish_topic, "schedule/state") != NULL) {
        return mqtt->last_publish_payload;
    }

    if (strstr(mqtt->prior_publish_topic, "schedule/state") != NULL) {
        return mqtt->prior_publish_payload;
    }

    return NULL;
}

static void drain_schedule(const mqtt_port_t *mqtt)
{
    while (mqtt_schedule_drain(mqtt)) {
    }
}

void test_mqtt_schedule_handle_set_publishes_state(void)
{
    const fake_mqtt_port_state_t *mqtt;
    const mqtt_port_t *port = fake_mqtt_port_get();
    const char *payload =
        "{\"hour\":8,\"min\":0,\"repeat_days\":[0,1,2,3,4,5,6],\"g\":30,\"enabled\":true}";

    setup_schedule_mqtt(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_TRUE(mqtt_schedule_handle(MQTT_ROUTE_CMD_SCHEDULE_SET,
                                            payload,
                                            strlen(payload)));
    drain_schedule(port);

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_NOT_NULL(mqtt_find_schedule_state_payload(mqtt));
    TEST_ASSERT_NOT_NULL(strstr(mqtt_find_schedule_state_payload(mqtt), "\"time\":\"08:00\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt_find_schedule_state_payload(mqtt), "\"g\":30"));
}

void test_mqtt_schedule_handle_enable_publishes_state(void)
{
    const fake_mqtt_port_state_t *mqtt;
    const mqtt_port_t *port = fake_mqtt_port_get();

    setup_schedule_mqtt(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_TRUE(schedule_set_global_enabled(false));
    drain_schedule(port);

    TEST_ASSERT_TRUE(mqtt_schedule_handle(MQTT_ROUTE_CMD_SCHEDULE_ENABLE,
                                            "{\"enabled\":true}",
                                            strlen("{\"enabled\":true}")));
    drain_schedule(port);

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_NOT_NULL(mqtt_find_schedule_state_payload(mqtt));
    TEST_ASSERT_NOT_NULL(strstr(mqtt_find_schedule_state_payload(mqtt), "\"enabled\":true"));
}

void test_mqtt_schedule_handle_rejects_invalid_set(void)
{
    setup_schedule_mqtt(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_FALSE(mqtt_schedule_handle(MQTT_ROUTE_CMD_SCHEDULE_SET,
                                             "{\"hour\":8}",
                                             strlen("{\"hour\":8}")));
}

void test_mqtt_schedule_handle_rejects_legacy_days(void)
{
    setup_schedule_mqtt(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_FALSE(mqtt_schedule_handle(MQTT_ROUTE_CMD_SCHEDULE_SET,
                                           "{\"hour\":8,\"min\":0,\"days\":127,\"g\":30}",
                                           strlen("{\"hour\":8,\"min\":0,\"days\":127,\"g\":30}")));
}

void test_mqtt_schedule_handle_set_repeat_days_bitmask(void)
{
    schedule_slot_config_t cfg;

    setup_schedule_mqtt(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_TRUE(mqtt_schedule_handle(MQTT_ROUTE_CMD_SCHEDULE_SET,
                                          "{\"hour\":9,\"min\":0,\"repeat_days\":[0,2],\"g\":25}",
                                          strlen("{\"hour\":9,\"min\":0,\"repeat_days\":[0,2],\"g\":25}")));
    TEST_ASSERT_TRUE(schedule_get_slot(0, &cfg, NULL));
    TEST_ASSERT_EQUAL_UINT8(9, cfg.hour);
    TEST_ASSERT_EQUAL_UINT8(5, cfg.days);
}

void test_mqtt_schedule_handle_enable_idempotent(void)
{
    setup_schedule_mqtt(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_TRUE(mqtt_schedule_handle(MQTT_ROUTE_CMD_SCHEDULE_ENABLE,
                                            "{\"enabled\":true}",
                                            strlen("{\"enabled\":true}")));
}

void test_mqtt_schedule_connect_snapshot_stages_publish(void)
{
    const mqtt_port_t *port = fake_mqtt_port_get();
    schedule_slot_config_t slot = {
        .hour = 7,
        .min = 30,
        .days = 127,
        .g = 20,
        .enabled = true,
    };

    setup_schedule_mqtt(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));

    mqtt_schedule_connect_snapshot();
    drain_schedule(port);

    TEST_ASSERT_NOT_NULL(strstr(mqtt_find_schedule_state_payload(fake_mqtt_port_state()),
                                "\"time\":\"07:30\""));
}
