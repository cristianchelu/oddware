/* Tests: spec/30-processes/app-logging.md § MQTT command ingress */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "app_event_port.h"
#include "dispense.h"
#include "fake_motor_port.h"
#include "motor_port_provider_host.h"
#include "app_mqtt_dispatch.h"

extern void fake_app_event_q_reset(void);

static char s_log_capture[768];
static size_t s_log_capture_len;

static void mqtt_dispatch_log_sink(const char *buf, size_t len, void *ctx)
{
    size_t room;

    (void)ctx;
    if (buf == NULL || len == 0) {
        return;
    }

    room = sizeof(s_log_capture) - s_log_capture_len;
    if (len > room) {
        len = room;
    }
    memcpy(s_log_capture + s_log_capture_len, buf, len);
    s_log_capture_len += len;
}

static void mqtt_dispatch_log_capture_begin(void)
{
    s_log_capture_len = 0;
    memset(s_log_capture, 0, sizeof(s_log_capture));
    app_log_set_sink(mqtt_dispatch_log_sink, NULL);
}

static void mqtt_dispatch_log_capture_end(void)
{
    app_log_clear_sink();
}

static void mqtt_dispatch_test_reset(void)
{
    motor_port_host_reset();
    fake_motor_port_reset();
    fake_app_event_q_reset();
    dispense_test_reset();
    app_event_port_init();
}

void test_app_mqtt_dispatch_logs_dispense_ingress_and_started(void)
{
    const char *topic = "petfeeder/ddeeff/cmd/dispense";
    const char *payload = "{}";

    mqtt_dispatch_test_reset();
    mqtt_dispatch_log_capture_begin();

    app_mqtt_dispatch(topic, payload, strlen(payload), "ddeeff");

    mqtt_dispatch_log_capture_end();
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "[mqtt]"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "cmd dispense topic=petfeeder/ddeeff/cmd/dispense"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "[dispense]"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "started portions=1"));
}

void test_app_mqtt_dispatch_logs_dispense_busy(void)
{
    const char *topic = "petfeeder/ddeeff/cmd/dispense";
    const char *payload = "{}";

    mqtt_dispatch_test_reset();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u, DISPENSE_SOURCE_MQTT));

    mqtt_dispatch_log_capture_begin();
    app_mqtt_dispatch(topic, payload, strlen(payload), "ddeeff");
    mqtt_dispatch_log_capture_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "cmd dispense topic="));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "[dispense]"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "busy"));
    TEST_ASSERT_NULL(strstr(s_log_capture, "started portions="));
}
