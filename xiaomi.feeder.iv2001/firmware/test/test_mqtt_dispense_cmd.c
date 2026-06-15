/* Tests: spec/30-processes/app-logging.md § Dispense diagnostics,
 *        spec/30-processes/mqtt-protocol.md § HA validation slice */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "app_event_port.h"
#include "dispense.h"
#include "fake_motor_port.h"
#include "motor_port_provider_host.h"
#include "mqtt_dispense_cmd.h"

extern void fake_app_event_q_reset(void);

static char s_log_capture[512];
static size_t s_log_capture_len;

static void dispense_log_sink(const char *buf, size_t len, void *ctx)
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

static void dispense_log_capture_begin(void)
{
    s_log_capture_len = 0;
    memset(s_log_capture, 0, sizeof(s_log_capture));
    app_log_set_sink(dispense_log_sink, NULL);
}

static void dispense_log_capture_end(void)
{
    app_log_clear_sink();
}

static void mqtt_dispense_cmd_test_reset(void)
{
    motor_port_host_reset();
    fake_motor_port_reset();
    fake_app_event_q_reset();
    dispense_test_reset();
    app_event_port_init();
}

void test_mqtt_dispense_cmd_logs_accepted(void)
{
    const char *topic = "petfeeder/ddeeff/cmd/dispense";
    const char *payload = "{}";

    mqtt_dispense_cmd_test_reset();
    dispense_log_capture_begin();

    mqtt_dispense_cmd_handle(topic, payload, strlen(payload), "ddeeff");

    dispense_log_capture_end();
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "[dispense]"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "remote dispense cmd topic=petfeeder/ddeeff/cmd/dispense"));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "remote dispense accepted portions=1"));
}

void test_mqtt_dispense_cmd_logs_busy(void)
{
    const char *topic = "petfeeder/ddeeff/cmd/dispense";
    const char *payload = "{}";

    mqtt_dispense_cmd_test_reset();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u));

    dispense_log_capture_begin();
    mqtt_dispense_cmd_handle(topic, payload, strlen(payload), "ddeeff");
    dispense_log_capture_end();

    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "remote dispense cmd topic="));
    TEST_ASSERT_NOT_NULL(strstr(s_log_capture, "remote dispense busy"));
    TEST_ASSERT_NULL(strstr(s_log_capture, "remote dispense accepted"));
}
