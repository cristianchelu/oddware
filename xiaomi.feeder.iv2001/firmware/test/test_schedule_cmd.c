/* Tests: spec/30-processes/web-ui.md, scheduler-engine.md */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "mqtt_route.h"
#include "schedule.h"
#include "schedule_cmd.h"

void test_schedule_cmd_set_valid_slot(void)
{
    schedule_slot_config_t cfg = {
        .hour = 8,
        .min = 0,
        .days = 0x7f,
        .g = 30,
        .enabled = true,
    };

    fake_config_port_reset();
    schedule_test_reset();
    schedule_init();

    TEST_ASSERT_EQUAL(SCHEDULE_CMD_OK, schedule_cmd_set(&cfg));
    TEST_ASSERT_EQUAL(1, schedule_slot_count());
}

void test_schedule_cmd_set_invalid_g(void)
{
    schedule_slot_config_t cfg = {
        .hour = 8,
        .min = 0,
        .days = 0x7f,
        .g = 4,
        .enabled = true,
    };

    fake_config_port_reset();
    schedule_test_reset();
    schedule_init();

    TEST_ASSERT_EQUAL(SCHEDULE_CMD_INVALID, schedule_cmd_set(&cfg));
}

void test_schedule_cmd_apply_json_set(void)
{
    const char *payload =
        "{\"hour\":9,\"min\":30,\"repeat_days\":[0,1,2,3,4,5,6],\"g\":25,\"enabled\":true}";

    fake_config_port_reset();
    schedule_test_reset();
    schedule_init();

    TEST_ASSERT_TRUE(schedule_cmd_apply_json(MQTT_ROUTE_CMD_SCHEDULE_SET, payload, strlen(payload)));
    TEST_ASSERT_EQUAL(1, schedule_slot_count());
}

void test_schedule_cmd_apply_json_rejects_legacy_days(void)
{
    const char *payload = "{\"hour\":8,\"min\":0,\"days\":127,\"g\":30}";

    fake_config_port_reset();
    schedule_test_reset();
    schedule_init();

    TEST_ASSERT_FALSE(schedule_cmd_apply_json(MQTT_ROUTE_CMD_SCHEDULE_SET, payload, strlen(payload)));
}

void test_schedule_cmd_enable_unchanged(void)
{
    fake_config_port_reset();
    schedule_test_reset();
    schedule_init();

    TEST_ASSERT_EQUAL(SCHEDULE_CMD_UNCHANGED, schedule_cmd_enable(schedule_global_enabled()));
}
