/* Tests: spec/30-processes/mqtt-protocol.md (Command topics) */

#include "unity.h"
#include "mqtt_route.h"

void test_route_dispense_command(void)
{
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_DISPENSE,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/dispense", "a4cf12"));
}

void test_route_ota_command(void)
{
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_OTA,
                      mqtt_route_classify("petfeeder/feeder1/cmd/ota", "feeder1"));
}

void test_route_unknown_topic(void)
{
    TEST_ASSERT_EQUAL(MQTT_ROUTE_UNKNOWN,
                      mqtt_route_classify("petfeeder/a4cf12/state", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_UNKNOWN,
                      mqtt_route_classify("petfeeder/other/cmd/ota", "a4cf12"));
}

void test_route_dispense_cancel(void)
{
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_DISPENSE_CANCEL,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/dispense/cancel", "a4cf12"));
}

void test_route_remaining_command_topics(void)
{
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_SCHEDULE_SET,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/schedule/set", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_SCHEDULE_DELETE,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/schedule/delete", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_SCHEDULE_TOGGLE,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/schedule/toggle", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_SCHEDULE_SKIP,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/schedule/skip", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_SCHEDULE_ENABLE,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/schedule/enable", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_SCHEDULE_TODAY,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/schedule/today", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_CALIBRATE,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/calibrate", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_DISPLAY,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/display", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_CONFIG,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/config", "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_CMD_REBOOT,
                      mqtt_route_classify("petfeeder/a4cf12/cmd/reboot", "a4cf12"));
}

void test_route_rejects_null_or_empty_inputs(void)
{
    TEST_ASSERT_EQUAL(MQTT_ROUTE_UNKNOWN, mqtt_route_classify(NULL, "a4cf12"));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_UNKNOWN, mqtt_route_classify("petfeeder/a4cf12/cmd/ota", NULL));
    TEST_ASSERT_EQUAL(MQTT_ROUTE_UNKNOWN, mqtt_route_classify("petfeeder/a4cf12/cmd/ota", ""));
}
