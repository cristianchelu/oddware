/* Tests: spec/30-processes/uart-console.md § motor commands */

#include <string.h>

#include "unity.h"

#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "cli_test_assert.h"
#include "fake_motor_port.h"
#include "motor_cli.h"
#include "motor_port_provider_host.h"

extern void fake_app_event_q_reset(void);

static void motor_cli_test_reset_all(void)
{
    motor_port_host_reset();
    fake_motor_port_reset();
    fake_app_event_q_reset();
    motor_cli_test_reset_timed();
    motor_cli_test_reset_park();
    app_test_reset();
    app_event_port_init();
}

static void assert_motor_cli_handle_fwd(uint8_t argc, char *argv[],
                                        const char *expect,
                                        uint8_t expect_rc)
{
    cli_test_reset();
    TEST_ASSERT_EQUAL(expect_rc, motor_cli_handle_fwd(argc, argv));
    assert_cli_body(expect);
}

static void assert_motor_cli_handle_rev(uint8_t argc, char *argv[],
                                        const char *expect,
                                        uint8_t expect_rc)
{
    cli_test_reset();
    TEST_ASSERT_EQUAL(expect_rc, motor_cli_handle_rev(argc, argv));
    assert_cli_body(expect);
}

void test_motor_cli_parse_duration_accepts_valid_range(void)
{
    uint32_t ms = 0u;

    fake_motor_port_reset();

    TEST_ASSERT_EQUAL(PORT_OK, motor_cli_parse_duration_ms("1", &ms));
    TEST_ASSERT_EQUAL(1u, ms);
    TEST_ASSERT_EQUAL(PORT_OK, motor_cli_parse_duration_ms("2", &ms));
    TEST_ASSERT_EQUAL(2u, ms);
    TEST_ASSERT_EQUAL(PORT_OK, motor_cli_parse_duration_ms("19999", &ms));
    TEST_ASSERT_EQUAL(19999u, ms);
    TEST_ASSERT_EQUAL(PORT_OK, motor_cli_parse_duration_ms("20000", &ms));
    TEST_ASSERT_EQUAL(20000u, ms);
}

void test_motor_cli_parse_duration_rejects_invalid(void)
{
    uint32_t ms = 0u;

    fake_motor_port_reset();

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("0", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("20001", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("abc", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("01", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("007", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms(" 100", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("100 ", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("+1", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("100foo", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, motor_cli_parse_duration_ms("12.5", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      motor_cli_parse_duration_ms("99999999999", &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      motor_cli_parse_duration_ms("4294967295", &ms));
}

void test_motor_cli_parse_rejects_null_args(void)
{
    uint32_t ms = 0u;

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      motor_cli_parse_duration_ms(NULL, &ms));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      motor_cli_parse_duration_ms("100", NULL));
}

void test_motor_cli_print_fail_fwd_busy(void)
{
    cli_test_reset();
    motor_cli_print_fail("fwd", PORT_ERR_BUSY);
    assert_cli_body("motor fwd failed (busy)");
}

void test_motor_cli_print_fail_rev_io(void)
{
    cli_test_reset();
    motor_cli_print_fail("rev", PORT_ERR_IO);
    assert_cli_body("motor rev failed (io)");
}

void test_motor_cli_print_fail_fwd_io(void)
{
    cli_test_reset();
    motor_cli_print_fail("fwd", PORT_ERR_IO);
    assert_cli_body("motor fwd failed (io)");
}

void test_motor_cli_print_fail_rev_busy(void)
{
    cli_test_reset();
    motor_cli_print_fail("rev", PORT_ERR_BUSY);
    assert_cli_body("motor rev failed (busy)");
}

void test_motor_cli_handle_fwd_async_start(void)
{
    char arg0[] = "500";
    char *argv[] = { arg0 };

    motor_cli_test_reset_all();
    assert_motor_cli_handle_fwd(1u, argv, "motor fwd started", 0u);
    TEST_ASSERT_EQUAL(1u, fake_motor_port_timed_fwd_calls());
    TEST_ASSERT_EQUAL(500u, fake_motor_port_last_timed_fwd_ms());
}

void test_motor_cli_handle_rev_async_start(void)
{
    char arg0[] = "300";
    char *argv[] = { arg0 };

    motor_cli_test_reset_all();
    assert_motor_cli_handle_rev(1u, argv, "motor rev started", 0u);
    TEST_ASSERT_EQUAL(1u, fake_motor_port_timed_rev_calls());
    TEST_ASSERT_EQUAL(300u, fake_motor_port_last_timed_rev_ms());
}

void test_motor_cli_handle_fwd_done_after_event(void)
{
    char arg0[] = "500";
    char *argv[] = { arg0 };
    app_event_t ev;

    motor_cli_test_reset_all();
    (void)motor_cli_handle_fwd(1u, argv);

    ev.type = EVT_TIMED_RUN_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    cli_test_reset();
    TEST_ASSERT_TRUE(app_step());
    assert_cli_body("motor fwd ok");
}

void test_motor_cli_handle_rev_fault_after_event(void)
{
    char arg0[] = "300";
    char *argv[] = { arg0 };
    app_event_t ev;

    motor_cli_test_reset_all();
    (void)motor_cli_handle_rev(1u, argv);

    ev.type = EVT_MOTOR_FAULT;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    cli_test_reset();
    TEST_ASSERT_TRUE(app_step());
    assert_cli_body("motor rev fault: stuck");
}

void test_motor_cli_handle_fwd_missing_duration_usage(void)
{
    motor_cli_test_reset_all();
    assert_motor_cli_handle_fwd(0u, NULL, "usage: motor fwd <ms>", 1u);
    assert_motor_cli_handle_rev(0u, NULL, "usage: motor rev <ms>", 1u);
}

void test_motor_cli_handle_fwd_invalid_duration(void)
{
    char arg0[] = "abc";
    char *argv[] = { arg0 };

    motor_cli_test_reset_all();
    assert_motor_cli_handle_fwd(1u, argv, "invalid duration", 1u);
    TEST_ASSERT_EQUAL(0u, fake_motor_port_timed_fwd_calls());
}

void test_motor_cli_handle_fwd_propagates_port_busy(void)
{
    char arg0[] = "100";
    char *argv[] = { arg0 };

    motor_cli_test_reset_all();
    fake_motor_port_set_timed_fwd_err(PORT_ERR_BUSY);
    assert_motor_cli_handle_fwd(1u, argv, "motor fwd failed (busy)", 1u);
}

void test_motor_cli_handle_rev_propagates_port_busy(void)
{
    char arg0[] = "100";
    char *argv[] = { arg0 };

    motor_cli_test_reset_all();
    fake_motor_port_set_timed_rev_err(PORT_ERR_BUSY);
    assert_motor_cli_handle_rev(1u, argv, "motor rev failed (busy)", 1u);
}

void test_motor_cli_handle_fwd_propagates_port_io(void)
{
    char arg0[] = "100";
    char *argv[] = { arg0 };

    motor_cli_test_reset_all();
    fake_motor_port_set_timed_fwd_err(PORT_ERR_IO);
    assert_motor_cli_handle_fwd(1u, argv, "motor fwd failed (io)", 1u);
}

void test_motor_cli_handle_fwd_argv_null_with_argc(void)
{
    motor_cli_test_reset_all();
    assert_motor_cli_handle_fwd(1u, NULL, "usage: motor fwd <ms>", 1u);
}

void test_motor_cli_handle_fwd_busy_when_motor_active(void)
{
    char arg0[] = "100";
    char *argv[] = { arg0 };

    motor_cli_test_reset_all();
    fake_motor_port_set_active(true);
    assert_motor_cli_handle_fwd(1u, argv, "motor fwd failed (busy)", 1u);
    TEST_ASSERT_EQUAL(0u, fake_motor_port_timed_fwd_calls());
}

void test_motor_cli_handle_fwd_busy_while_waiting(void)
{
    char arg0[] = "100";
    char *argv[] = { arg0 };

    motor_cli_test_reset_all();
    (void)motor_cli_handle_fwd(1u, argv);
    cli_test_reset();
    assert_motor_cli_handle_fwd(1u, argv, "motor fwd failed (busy)", 1u);
    TEST_ASSERT_EQUAL(1u, fake_motor_port_timed_fwd_calls());
}

void test_motor_cli_handle_fwd_min_duration(void)
{
    char arg0[] = "1";
    char *argv[] = { arg0 };

    motor_cli_test_reset_all();
    assert_motor_cli_handle_fwd(1u, argv, "motor fwd started", 0u);
    TEST_ASSERT_EQUAL(1u, fake_motor_port_last_timed_fwd_ms());
}
