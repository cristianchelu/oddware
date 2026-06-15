/* Tests: spec/30-processes/dispense-cycle.md, uart-console.md § dispense */

#include "unity.h"

#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "cli_test_assert.h"
#include "dispense.h"
#include "dispense_cli.h"
#include "fake_motor_port.h"
#include "motor_jam.h"
#include "port_err.h"
#include "motor_port_provider_host.h"

extern void fake_app_event_q_reset(void);

static void dispense_test_reset_all(void)
{
    motor_port_host_reset();
    fake_motor_port_reset();
    fake_app_event_q_reset();
    dispense_test_reset();
    dispense_cli_test_reset();
    app_test_reset();
    app_event_port_init();
}

void test_dispense_submit_portions_posts_request_event(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(3u));
    TEST_ASSERT_TRUE(dispense_is_active());

    TEST_ASSERT_EQUAL(0u, fake_motor_port_burst_calls());

    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
    TEST_ASSERT_EQUAL(3u, fake_motor_port_last_pulse_target());
    TEST_ASSERT_EQUAL(MOTOR_BURST_TIMEOUT_MS, fake_motor_port_last_timeout_ms());
}

void test_dispense_submit_one_portion_pulse_target(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(1u));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_last_pulse_target());
}

void test_dispense_submit_rejects_invalid_portions(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_INVALID, dispense_submit_portions(0u));
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_INVALID, dispense_submit_portions(16u));
    TEST_ASSERT_FALSE(dispense_is_active());
    TEST_ASSERT_EQUAL(0u, fake_motor_port_burst_calls());
}

void test_dispense_submit_busy_while_job_active(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u));
    TEST_ASSERT_TRUE(dispense_is_active());
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_BUSY, dispense_submit_portions(1u));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
}

void test_dispense_submit_busy_when_motor_active(void)
{
    dispense_test_reset_all();
    fake_motor_port_set_active(true);
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_BUSY, dispense_submit_portions(1u));
}

void test_dispense_job_completes_on_burst_done(void)
{
    app_event_t ev;

    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(3u));
    TEST_ASSERT_TRUE(app_step());

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());

    TEST_ASSERT_FALSE(dispense_is_active());
}

void test_dispense_job_aborts_on_motor_fault(void)
{
    app_event_t ev;

    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u));
    TEST_ASSERT_TRUE(app_step());

    ev.type = EVT_MOTOR_FAULT;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());

    TEST_ASSERT_FALSE(dispense_is_active());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
}

void test_dispense_recovers_when_motor_idle_without_burst_event(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());

    fake_motor_port_set_active(false);
    dispense_poll();

    TEST_ASSERT_FALSE(dispense_is_active());
}

void test_app_prioritizes_burst_done_before_display_tick(void)
{
    app_event_t ev;

    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(1u));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());
    fake_motor_port_set_active(true);

    ev.type = EVT_DISPLAY_TICK;
    ev.u.display_tick.now_ms = 1000u;
    TEST_ASSERT_TRUE(app_event_post(&ev));

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));

    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_FALSE(dispense_is_active());
}

void test_dispense_cli_parse_portions_valid_range(void)
{
    uint8_t portions;

    TEST_ASSERT_EQUAL(PORT_OK, dispense_cli_parse_portions("1", &portions));
    TEST_ASSERT_EQUAL_UINT8(1u, portions);
    TEST_ASSERT_EQUAL(PORT_OK, dispense_cli_parse_portions("15", &portions));
    TEST_ASSERT_EQUAL_UINT8(15u, portions);
}

void test_dispense_cli_parse_portions_rejects_invalid(void)
{
    uint8_t portions;

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_portions("0", &portions));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_portions("16", &portions));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_portions("01", &portions));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_portions("abc", &portions));
}
