/* Tests: spec/30-processes/uart-console.md § dispense, motor park */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "dispense_cli.h"
#include "fake_motor_port.h"
#include "motor_cli.h"
#include "motor_port_provider_host.h"
#include "motor_jam.h"

extern void fake_app_event_q_reset(void);

static void capture_stdout_begin(FILE **saved, FILE **cap)
{
    *saved = stdout;
    *cap = tmpfile();
    TEST_ASSERT_NOT_NULL(*cap);
    stdout = *cap;
}

static void capture_stdout_end(FILE *saved, FILE *cap, char *buf, size_t len)
{
    fflush(stdout);
    rewind(cap);
    if (buf != NULL && len > 0u) {
        if (fgets(buf, (int)len, cap) == NULL) {
            buf[0] = '\0';
        }
    }
    stdout = saved;
    fclose(cap);
}

static void dispense_cli_test_reset_all(void)
{
    motor_port_host_reset();
    fake_motor_port_reset();
    fake_app_event_q_reset();
    dispense_cli_test_reset();
    motor_cli_test_reset_park();
    app_test_reset();
    app_event_port_init();
}

void test_dispense_cli_posts_start_without_blocking_motor_run(void)
{
    char buf[48];
    FILE *saved;
    FILE *cap;

    dispense_cli_test_reset_all();
    capture_stdout_begin(&saved, &cap);
    TEST_ASSERT_EQUAL(0u, dispense_cli_handle(0u, NULL));
    capture_stdout_end(saved, cap, buf, sizeof(buf));

    TEST_ASSERT_EQUAL_STRING("dispense started\r\n", buf);
    TEST_ASSERT_EQUAL(0u, fake_motor_port_run_calls());
    TEST_ASSERT_EQUAL(0u, fake_motor_port_burst_calls());

    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_last_pulse_target());
    TEST_ASSERT_EQUAL(MOTOR_BURST_TIMEOUT_MS, fake_motor_port_last_timeout_ms());
}

void test_dispense_cli_busy_when_motor_active(void)
{
    char buf[48];
    FILE *saved;
    FILE *cap;

    dispense_cli_test_reset_all();
    fake_motor_port_set_active(true);
    capture_stdout_begin(&saved, &cap);
    TEST_ASSERT_EQUAL(1u, dispense_cli_handle(0u, NULL));
    capture_stdout_end(saved, cap, buf, sizeof(buf));

    TEST_ASSERT_EQUAL_STRING("dispense busy\r\n", buf);
    TEST_ASSERT_EQUAL(0u, fake_motor_port_burst_calls());
}

void test_dispense_cli_done_after_burst_event(void)
{
    char buf[48];
    app_event_t ev;
    FILE *saved;
    FILE *cap;

    dispense_cli_test_reset_all();
    (void)dispense_cli_handle(0u, NULL);
    (void)app_step();

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    capture_stdout_begin(&saved, &cap);
    TEST_ASSERT_TRUE(app_step());
    capture_stdout_end(saved, cap, buf, sizeof(buf));

    TEST_ASSERT_EQUAL_STRING("dispense done\r\n", buf);
}

void test_dispense_cli_fault_on_motor_fault_event(void)
{
    char buf[48];
    app_event_t ev;
    FILE *saved;
    FILE *cap;

    dispense_cli_test_reset_all();
    (void)dispense_cli_handle(0u, NULL);
    (void)app_step();

    ev.type = EVT_MOTOR_FAULT;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    capture_stdout_begin(&saved, &cap);
    TEST_ASSERT_TRUE(app_step());
    capture_stdout_end(saved, cap, buf, sizeof(buf));

    TEST_ASSERT_EQUAL_STRING("dispense fault: stuck\r\n", buf);
}

void test_motor_park_cli_async_start(void)
{
    char buf[48];
    FILE *saved;
    FILE *cap;

    dispense_cli_test_reset_all();
    capture_stdout_begin(&saved, &cap);
    TEST_ASSERT_EQUAL(0u, motor_cli_handle_park());
    capture_stdout_end(saved, cap, buf, sizeof(buf));

    TEST_ASSERT_EQUAL_STRING("motor park started\r\n", buf);
    TEST_ASSERT_EQUAL(1u, fake_motor_port_park_calls());
    TEST_ASSERT_EQUAL(0u, fake_motor_port_run_calls());
}
