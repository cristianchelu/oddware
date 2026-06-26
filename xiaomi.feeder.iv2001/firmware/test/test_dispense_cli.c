/* Tests: spec/30-processes/uart-console.md § dispense, motor park */

#include "unity.h"

#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "cli_test_assert.h"
#include "dispense.h"
#include "auto_tare.h"
#include "dispense_cli.h"
#include "fake_config_port.h"
#include "fake_motor_port.h"
#include "feed_config.h"
#include "fake_weight_port.h"
#include "motor_cli.h"
#include "motor_port_provider_host.h"
#include "motor_jam.h"

extern void fake_app_event_q_reset(void);

static void dispense_cli_advance_settle(uint32_t start_ms)
{
    dispense_poll(start_ms);
    dispense_poll(start_ms + DISPENSE_SETTLE_MS);
}

static void dispense_cli_test_reset_all(void)
{
    motor_port_host_reset();
    fake_motor_port_reset();
    fake_app_event_q_reset();
    dispense_test_reset();
    dispense_cli_test_reset();
    motor_cli_test_reset_timed();
    motor_cli_test_reset_park();
    app_test_reset();
    app_event_port_init();
}

void test_dispense_cli_posts_start_without_blocking_motor_run(void)
{
    dispense_cli_test_reset_all();
    cli_test_reset();
    TEST_ASSERT_EQUAL(0u, dispense_cli_handle_default(0u, NULL));
    assert_log_body("dispense", "started portions=1");

    TEST_ASSERT_EQUAL(0u, fake_motor_port_timed_fwd_calls());
    TEST_ASSERT_EQUAL(0u, fake_motor_port_burst_calls());

    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_last_pulse_target());
    TEST_ASSERT_EQUAL(MOTOR_BURST_TIMEOUT_MS, fake_motor_port_last_timeout_ms());
}

void test_dispense_cli_portions_posts_burst_target(void)
{
    char *argv[] = { "3" };

    dispense_cli_test_reset_all();
    cli_test_reset();
    TEST_ASSERT_EQUAL(0u, dispense_cli_handle_portions(1u, argv));
    assert_log_body("dispense", "started portions=3");
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
    TEST_ASSERT_EQUAL(3u, fake_motor_port_last_pulse_target());
}

void test_dispense_cli_portions_usage_on_bad_arg(void)
{
    char *argv[] = { "16" };

    dispense_cli_test_reset_all();
    cli_test_reset();
    TEST_ASSERT_EQUAL(1u, dispense_cli_handle_portions(1u, argv));
    assert_cli_body("dispense usage: portions <1-15>");
}

void test_dispense_cli_busy_when_motor_active(void)
{
    dispense_cli_test_reset_all();
    fake_motor_port_set_active(true);
    cli_test_reset();
    TEST_ASSERT_EQUAL(1u, dispense_cli_handle_default(0u, NULL));
    assert_log_body("dispense", "busy");
    TEST_ASSERT_EQUAL(0u, fake_motor_port_burst_calls());
}

void test_dispense_cli_done_after_burst_event(void)
{
    app_event_t ev;

    dispense_cli_test_reset_all();
    (void)dispense_cli_handle_default(0u, NULL);
    (void)app_step();

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    cli_test_reset();
    TEST_ASSERT_TRUE(app_step());
    dispense_cli_advance_settle(1000u);
    assert_cli_body("dispense done");
}

void test_dispense_cli_fault_on_motor_fault_event(void)
{
    app_event_t ev;

    dispense_cli_test_reset_all();
    (void)dispense_cli_handle_default(0u, NULL);
    (void)app_step();

    ev.type = EVT_MOTOR_FAULT;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    cli_test_reset();
    TEST_ASSERT_TRUE(app_step());
    dispense_cli_advance_settle(1000u);
    assert_cli_body("dispense fault: stuck");
}

void test_dispense_cli_grams_posts_gram_request(void)
{
    char *argv[] = { "25" };

    dispense_cli_test_reset_all();
    cli_test_reset();
    TEST_ASSERT_EQUAL(0u, dispense_cli_handle_grams(1u, argv));
    assert_log_body("dispense", "started grams=25");
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(3u, fake_motor_port_last_pulse_target());
}

void test_dispense_cli_grams_usage_on_bad_arg(void)
{
    char *argv[] = { "4" };

    dispense_cli_test_reset_all();
    cli_test_reset();
    TEST_ASSERT_EQUAL(1u, dispense_cli_handle_grams(1u, argv));
    assert_cli_body("dispense usage: grams <5-150>");
}

void test_dispense_cli_grams_underfill_fault(void)
{
    app_event_t ev;
    char *argv[] = { "30" };
    uint32_t t = 10000u;

    dispense_cli_test_reset_all();
    fake_config_port_reset();
    fake_weight_port_reset();
    TEST_ASSERT_TRUE(feed_config_mode_set(DISPENSE_MODE_COMPENSATED));
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    auto_tare_anchor(100);
    app_bowl_grams_notify_read(100, true, 0u);
    fake_weight_port_set_read_grams(100);

    cli_test_reset();
    TEST_ASSERT_EQUAL(0u, dispense_cli_handle_grams(1u, argv));
    TEST_ASSERT_TRUE(app_step());

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    fake_weight_port_set_read_grams(110);
    dispense_cli_advance_settle(t);
    t += 20000u;

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    fake_weight_port_set_read_grams(115);
    dispense_cli_advance_settle(t);
    t += 20000u;

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    fake_weight_port_set_read_grams(118);
    cli_test_reset();
    dispense_cli_advance_settle(t);

    assert_cli_body("dispense fault: underfill");
}

void test_motor_park_cli_async_start(void)
{
    dispense_cli_test_reset_all();
    cli_test_reset();
    TEST_ASSERT_EQUAL(0u, motor_cli_handle_park());
    assert_cli_body("motor park started");
    TEST_ASSERT_EQUAL(1u, fake_motor_port_park_calls());
    TEST_ASSERT_EQUAL(0u, fake_motor_port_timed_fwd_calls());
}
