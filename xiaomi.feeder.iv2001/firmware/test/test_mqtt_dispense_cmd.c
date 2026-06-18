/* Tests: spec/30-processes/mqtt-protocol.md § HA validation slice,
 *        spec/30-processes/app-logging.md § Dispense diagnostics */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "app_event_port.h"
#include "dispense.h"
#include "fake_motor_port.h"
#include "motor_port_provider_host.h"
#include "mqtt_dispense_cmd.h"

extern void fake_app_event_q_reset(void);

static void mqtt_dispense_cmd_test_reset(void)
{
    motor_port_host_reset();
    fake_motor_port_reset();
    fake_app_event_q_reset();
    dispense_test_reset();
    app_event_port_init();
    app_log_test_reset();
}

void test_mqtt_dispense_cmd_submits_portions(void)
{
    const char *topic = "petfeeder/ddeeff/cmd/dispense";
    const char *payload = "{}";

    mqtt_dispense_cmd_test_reset();

    mqtt_dispense_cmd_handle(topic, payload, strlen(payload), "ddeeff");

    TEST_ASSERT_TRUE(dispense_is_active());
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "[dispense]"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "started portions=1"));
}

void test_dispense_submit_logs_busy(void)
{
    mqtt_dispense_cmd_test_reset();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u, DISPENSE_SOURCE_MQTT));

    app_log_test_reset();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_BUSY, dispense_submit_portions(1u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "[dispense]"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "busy"));
}
