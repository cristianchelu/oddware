/* Tests: spec/30-processes/dispense-cycle.md, spec/30-processes/motor-index.md,
 * spec/30-processes/jam-detection.md */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "app_event.h"
#include "app_event_port.h"
#include "board_gpio_iv2001.h"
#include "fake_adc_port.h"
#include "fake_gpio_expander_port.h"
#include "fake_wfci_bus_port.h"
#include "fake_motor_index_port.h"
#include "fake_time.h"
#include "motor_ctrl.h"
#include "motor_ctrl_test.h"
#include "motor_index_port_provider_host.h"
#include "motor_port_provider_host.h"
#include "motor_jam.h"
#include "motor_limits.h"

extern void fake_app_event_q_reset(void);
extern void freertos_notify_test_reset(void);

static void motor_ctrl_test_setup(void)
{
    motor_index_port_host_reset();
    motor_port_host_reset();
    fake_gpio_expander_reset();
    fake_wfci_bus_reset();
    fake_motor_index_port_reset();
    fake_adc_port_reset();
    fake_time_reset();
    freertos_notify_test_reset();
    fake_app_event_q_reset();
    app_event_port_init();
    motor_ctrl_start();
    motor_ctrl_test_reset();
}

static void motor_ctrl_run_until_idle(uint32_t max_polls)
{
    uint32_t i;

    for (i = 0u; i < max_polls; i++) {
        motor_ctrl_test_poll();
        if (!motor_ctrl_is_active()) {
            break;
        }
    }
}

static void capture_stdout_begin(FILE **saved, FILE **cap)
{
    *saved = stdout;
    *cap = tmpfile();
    TEST_ASSERT_NOT_NULL(*cap);
    stdout = *cap;
}

static void capture_stdout_end(FILE *saved, FILE *cap, char *buf, size_t len)
{
    size_t n = 0u;
    int ch;

    fflush(stdout);
    rewind(cap);
    if (buf != NULL && len > 0u) {
        while (n + 1u < len && (ch = fgetc(cap)) != EOF) {
            buf[n++] = (char)ch;
        }
        buf[n] = '\0';
    }
    stdout = saved;
    fclose(cap);
}

static bool app_event_pending_type(app_event_type_t expect)
{
    app_event_t ev;

    if (!app_event_try_receive(&ev)) {
        return false;
    }

    TEST_ASSERT_EQUAL(expect, ev.type);
    app_event_release(&ev);
    return true;
}

void test_motor_ctrl_park_ends_on_beam_open(void)
{
    motor_ctrl_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_park(MOTOR_PARK_MAX_PULSES_DEFAULT));

    motor_ctrl_test_poll();
    fake_motor_index_port_set_beam_open(true);
    fake_time_advance_ms(MOTOR_PARK_BEAM_POLL_MS + 1u);
    motor_ctrl_run_until_idle(50u);

    TEST_ASSERT_FALSE(motor_ctrl_is_active());
    TEST_ASSERT_TRUE(app_event_pending_type(EVT_PARK_DONE));
}

void test_motor_ctrl_park_done_immediately_when_already_open(void)
{
    char log[128];
    FILE *saved;
    FILE *cap;

    motor_ctrl_test_setup();
    fake_motor_index_port_set_beam_open(true);
    capture_stdout_begin(&saved, &cap);
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_park(MOTOR_PARK_MAX_PULSES_DEFAULT));
    motor_ctrl_test_poll();
    capture_stdout_end(saved, cap, log, sizeof(log));

    TEST_ASSERT_FALSE(motor_ctrl_is_active());
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_FALSE(fake_motor_index_port_get_led());
    TEST_ASSERT_TRUE(app_event_pending_type(EVT_PARK_DONE));
    TEST_ASSERT_NOT_NULL(strstr(log, "park: already aligned (beam open)"));
}

void test_motor_ctrl_session_faults_on_20s_cap_when_stop_fails(void)
{
    motor_ctrl_test_setup();
  /* 1=reset stop, 2=PH, 3=EN on start; stop on session end is call 4+. */
    fake_gpio_expander_set_set_pin_fail_range(4u, 0u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));
    motor_ctrl_test_poll();
    TEST_ASSERT_TRUE(motor_ctrl_is_active());

    fake_time_advance_ms(MOTOR_RUN_MS_MAX + MOTOR_CTRL_LOOP_SLICE_MS);
    motor_ctrl_run_until_idle(200u);

    TEST_ASSERT_FALSE(motor_ctrl_is_active());
    TEST_ASSERT_TRUE(app_event_pending_type(EVT_MOTOR_FAULT));
}

void test_motor_ctrl_burst_faults_after_index_timeout_retries(void)
{
    char log[2048];
    FILE *saved;
    FILE *cap;

    motor_ctrl_test_setup();
    capture_stdout_begin(&saved, &cap);
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));

    motor_ctrl_test_poll();
    fake_time_advance_ms(MOTOR_BURST_TIMEOUT_MS + MOTOR_CTRL_LOOP_SLICE_MS);
    motor_ctrl_run_until_idle(5000u);
    capture_stdout_end(saved, cap, log, sizeof(log));

    TEST_ASSERT_FALSE(motor_ctrl_is_active());
    TEST_ASSERT_TRUE(app_event_pending_type(EVT_MOTOR_FAULT));
    TEST_ASSERT_NOT_NULL(strstr(log, "jam: index timeout"));
    TEST_ASSERT_NOT_NULL(strstr(log, "antijam: retry"));
    TEST_ASSERT_NOT_NULL(
        strstr(log, "stuck: antijam retries exhausted (last jam: index timeout)"));
}

void test_motor_ctrl_logs_sustained_adc_jam(void)
{
    char log[1024];
    FILE *saved;
    FILE *cap;
    uint32_t i;

    motor_ctrl_test_setup();
    fake_adc_port_set_motor_ma(600u);
    capture_stdout_begin(&saved, &cap);
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));
    motor_ctrl_test_poll();

    for (i = 0u; i < (MOTOR_JAM_SUSTAINED_MS / MOTOR_CTRL_LOOP_SLICE_MS) + 1u;
         i++) {
        motor_ctrl_test_poll();
        if (!motor_ctrl_is_active()) {
            break;
        }
    }
    capture_stdout_end(saved, cap, log, sizeof(log));

    TEST_ASSERT_NOT_NULL(strstr(log, "jam: adc sustained 600 mA"));
    TEST_ASSERT_NOT_NULL(strstr(log, "antijam: retry 1/3"));
}

void test_motor_ctrl_session_fault_logs_timeout(void)
{
    char log[512];
    FILE *saved;
    FILE *cap;

    motor_ctrl_test_setup();
    fake_gpio_expander_set_set_pin_fail_range(4u, 0u, PORT_ERR_IO);
    capture_stdout_begin(&saved, &cap);
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));
    motor_ctrl_test_poll();
    fake_time_advance_ms(MOTOR_RUN_MS_MAX + MOTOR_CTRL_LOOP_SLICE_MS);
    motor_ctrl_run_until_idle(200u);
    capture_stdout_end(saved, cap, log, sizeof(log));

    TEST_ASSERT_NOT_NULL(strstr(log, "stuck: session timeout (20000 ms)"));
}

void test_motor_ctrl_timed_forward_posts_done(void)
{
    motor_ctrl_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, motor_ctrl_request_timed_forward_ms(500u));
    motor_ctrl_test_poll();
    TEST_ASSERT_TRUE(motor_ctrl_is_active());
    fake_time_advance_ms(500u + MOTOR_CTRL_LOOP_SLICE_MS);
    motor_ctrl_run_until_idle(50u);
    TEST_ASSERT_TRUE(app_event_pending_type(EVT_TIMED_RUN_DONE));
}

void test_motor_ctrl_timed_reverse_posts_done(void)
{
    motor_ctrl_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, motor_ctrl_request_timed_reverse_ms(300u));
    motor_ctrl_test_poll();
    TEST_ASSERT_TRUE(motor_ctrl_is_active());
    fake_time_advance_ms(300u + MOTOR_CTRL_LOOP_SLICE_MS);
    motor_ctrl_run_until_idle(50u);
    TEST_ASSERT_TRUE(app_event_pending_type(EVT_TIMED_RUN_DONE));
}

void test_motor_ctrl_timed_run_rejects_when_active(void)
{
    motor_ctrl_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));
    motor_ctrl_test_poll();
    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, motor_ctrl_request_timed_forward_ms(100u));
}

void test_motor_ctrl_park_survives_burst_index_timeout(void)
{
    motor_ctrl_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_park(MOTOR_PARK_MAX_PULSES_DEFAULT));

    motor_ctrl_test_poll();
    TEST_ASSERT_TRUE(motor_ctrl_is_active());

    fake_time_advance_ms(MOTOR_BURST_TIMEOUT_MS + MOTOR_CTRL_LOOP_SLICE_MS);
    motor_ctrl_test_poll();

    TEST_ASSERT_TRUE(motor_ctrl_is_active());

    fake_motor_index_port_set_beam_open(true);
    fake_time_advance_ms(MOTOR_PARK_BEAM_POLL_MS + 1u);
    motor_ctrl_run_until_idle(50u);

    TEST_ASSERT_FALSE(motor_ctrl_is_active());
    TEST_ASSERT_TRUE(app_event_pending_type(EVT_PARK_DONE));
}

