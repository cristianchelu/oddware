/* Tests: spec/30-processes/uart-console.md § motor commands */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "fake_motor_port.h"
#include "motor_cli.h"
#include "motor_port_provider_host.h"

static void assert_motor_cli_fail_msg(const char *verb, port_err_t err,
                                      const char *expect)
{
    char buf[96];
    FILE *saved = stdout;
    FILE *cap = tmpfile();

    TEST_ASSERT_NOT_NULL(cap);
    stdout = cap;
    motor_cli_print_fail(verb, err);
    fflush(stdout);
    rewind(cap);
    TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), cap));
    stdout = saved;
    fclose(cap);
    TEST_ASSERT_EQUAL_STRING(expect, buf);
}

static void assert_motor_cli_handle_run(const char *verb,
                                        port_err_t (*run_ms)(uint32_t),
                                        uint8_t argc, char *argv[],
                                        const char *expect,
                                        uint8_t expect_rc)
{
    char buf[96];
    FILE *saved = stdout;
    FILE *cap = tmpfile();
    uint8_t rc;

    TEST_ASSERT_NOT_NULL(cap);
    stdout = cap;
    rc = motor_cli_handle_run(verb, run_ms, argc, argv);
    fflush(stdout);
    rewind(cap);
    TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), cap));
    stdout = saved;
    fclose(cap);
    TEST_ASSERT_EQUAL_STRING(expect, buf);
    TEST_ASSERT_EQUAL(expect_rc, rc);
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

void test_motor_cli_run_fwd_ms_delegates_to_port(void)
{
    fake_motor_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_cli_run_fwd_ms(500u));
    TEST_ASSERT_EQUAL(1u, fake_motor_port_run_calls());
    TEST_ASSERT_EQUAL(500u, fake_motor_port_last_duration_ms());
}

void test_motor_cli_run_fwd_ms_propagates_port_error(void)
{
    fake_motor_port_reset();
    fake_motor_port_set_run_err(PORT_ERR_BUSY);

    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, motor_cli_run_fwd_ms(100u));
}

void test_motor_cli_run_rev_ms_delegates_to_port(void)
{
    fake_motor_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_cli_run_rev_ms(300u));
    TEST_ASSERT_EQUAL(1u, fake_motor_port_reverse_calls());
    TEST_ASSERT_EQUAL(300u, fake_motor_port_last_reverse_duration_ms());
}

void test_motor_cli_run_rev_ms_propagates_port_error(void)
{
    fake_motor_port_reset();
    fake_motor_port_set_reverse_err(PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, motor_cli_run_rev_ms(100u));
}

void test_motor_cli_print_fail_fwd_busy(void)
{
    assert_motor_cli_fail_msg("fwd", PORT_ERR_BUSY, "motor fwd failed (busy)\r\n");
}

void test_motor_cli_print_fail_rev_io(void)
{
    assert_motor_cli_fail_msg("rev", PORT_ERR_IO, "motor rev failed (io)\r\n");
}

void test_motor_cli_print_fail_fwd_io(void)
{
    assert_motor_cli_fail_msg("fwd", PORT_ERR_IO, "motor fwd failed (io)\r\n");
}

void test_motor_cli_print_fail_rev_busy(void)
{
    assert_motor_cli_fail_msg("rev", PORT_ERR_BUSY, "motor rev failed (busy)\r\n");
}

void test_motor_cli_handle_run_fwd_ok(void)
{
    char arg0[] = "500";
    char *argv[] = { arg0 };

    fake_motor_port_reset();
    assert_motor_cli_handle_run("fwd", motor_cli_run_fwd_ms, 1u, argv,
                                "motor fwd ok\r\n", 0u);
    TEST_ASSERT_EQUAL(500u, fake_motor_port_last_duration_ms());
}

void test_motor_cli_handle_run_rev_ok(void)
{
    char arg0[] = "300";
    char *argv[] = { arg0 };

    fake_motor_port_reset();
    assert_motor_cli_handle_run("rev", motor_cli_run_rev_ms, 1u, argv,
                                "motor rev ok\r\n", 0u);
    TEST_ASSERT_EQUAL(300u, fake_motor_port_last_reverse_duration_ms());
}

void test_motor_cli_handle_run_missing_duration_usage(void)
{
    assert_motor_cli_handle_run("fwd", motor_cli_run_fwd_ms, 0u, NULL,
                                "usage: motor fwd <ms>\r\n", 1u);
    assert_motor_cli_handle_run("rev", motor_cli_run_rev_ms, 0u, NULL,
                                "usage: motor rev <ms>\r\n", 1u);
}

void test_motor_cli_handle_run_invalid_duration(void)
{
    char arg0[] = "abc";
    char *argv[] = { arg0 };

    fake_motor_port_reset();
    assert_motor_cli_handle_run("fwd", motor_cli_run_fwd_ms, 1u, argv,
                                "invalid duration\r\n", 1u);
    TEST_ASSERT_EQUAL(0u, fake_motor_port_run_calls());
}

void test_motor_cli_handle_run_propagates_port_busy(void)
{
    char arg0[] = "100";
    char *argv[] = { arg0 };

    fake_motor_port_reset();
    fake_motor_port_set_run_err(PORT_ERR_BUSY);
    assert_motor_cli_handle_run("fwd", motor_cli_run_fwd_ms, 1u, argv,
                                "motor fwd failed (busy)\r\n", 1u);
}

void test_motor_cli_handle_run_propagates_rev_port_busy(void)
{
    char arg0[] = "100";
    char *argv[] = { arg0 };

    fake_motor_port_reset();
    fake_motor_port_set_reverse_err(PORT_ERR_BUSY);
    assert_motor_cli_handle_run("rev", motor_cli_run_rev_ms, 1u, argv,
                                "motor rev failed (busy)\r\n", 1u);
}

void test_motor_cli_handle_run_propagates_port_io(void)
{
    char arg0[] = "100";
    char *argv[] = { arg0 };

    fake_motor_port_reset();
    fake_motor_port_set_run_err(PORT_ERR_IO);
    assert_motor_cli_handle_run("fwd", motor_cli_run_fwd_ms, 1u, argv,
                                "motor fwd failed (io)\r\n", 1u);
}

void test_motor_cli_handle_run_argv_null_with_argc(void)
{
    assert_motor_cli_handle_run("fwd", motor_cli_run_fwd_ms, 1u, NULL,
                                "usage: motor fwd <ms>\r\n", 1u);
}

void test_motor_cli_handle_run_fwd_min_duration(void)
{
    char arg0[] = "1";
    char *argv[] = { arg0 };

    fake_motor_port_reset();
    assert_motor_cli_handle_run("fwd", motor_cli_run_fwd_ms, 1u, argv,
                                "motor fwd ok\r\n", 0u);
    TEST_ASSERT_EQUAL(1u, fake_motor_port_last_duration_ms());
}
