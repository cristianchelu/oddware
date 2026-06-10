/* Tests: spec/30-processes/uart-console.md § weigh commands */

#include "unity.h"

#include "fake_weight_port.h"
#include "weigh_cli.h"

void test_weigh_cli_run_read_uses_port(void)
{
    const fake_weight_op_t *ops;
    size_t count;
    int32_t grams;

    fake_weight_port_reset();
    fake_weight_port_set_read_grams(250);

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cli_run_read(&grams));
    TEST_ASSERT_EQUAL(250, grams);

    ops = fake_weight_port_ops(&count);
    TEST_ASSERT_EQUAL(1u, count);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_READ_GRAMS, ops[0].kind);
}

void test_weigh_cli_run_cal_span_invokes_port(void)
{
    const fake_weight_op_t *ops;
    size_t count;

    fake_weight_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cli_run_cal_span());

    ops = fake_weight_port_ops(&count);
    TEST_ASSERT_EQUAL(1u, count);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_CAL_SPAN, ops[0].kind);
}

void test_weigh_cli_cal_status_name(void)
{
    TEST_ASSERT_EQUAL_STRING("success", weigh_cli_cal_status_name(WEIGHT_CAL_SUCCESS));
    TEST_ASSERT_EQUAL_STRING("idle", weigh_cli_cal_status_name(WEIGHT_CAL_IDLE));
}

void test_weigh_cli_read_after_power_off_returns_not_found(void)
{
    int32_t grams;

    fake_weight_port_reset();
    fake_weight_port_set_scale_off(true);
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_FOUND, weigh_cli_run_read(&grams));
}

void test_weigh_cli_power_on_after_power_off(void)
{
    const fake_weight_op_t *ops;
    size_t count;
    int32_t grams;

    fake_weight_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cli_run_power_off());
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_FOUND, weigh_cli_run_read(&grams));
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cli_run_power_on());
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cli_run_read(&grams));

    ops = fake_weight_port_ops(&count);
    TEST_ASSERT_EQUAL(3u, count);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_POWER_OFF, ops[0].kind);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_POWER_ON, ops[1].kind);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_READ_GRAMS, ops[2].kind);
}

void test_weigh_cli_print_scale_off_message(void)
{
    TEST_ASSERT_TRUE(weigh_cli_print_scale_off("read", PORT_ERR_NOT_FOUND));
}

void test_weigh_cli_print_read_fail_no_calibration(void)
{
    fake_weight_port_reset();
    fake_weight_port_set_read_err(PORT_ERR_NOT_SUPPORTED);
    fake_weight_port_set_cal_status(WEIGHT_CAL_UNCALIBRATED);
    TEST_ASSERT_TRUE(weigh_cli_print_read_fail(PORT_ERR_NOT_SUPPORTED));
}

void test_weigh_cli_print_read_fail_incomplete_calibration(void)
{
    fake_weight_port_reset();
    fake_weight_port_set_read_err(PORT_ERR_NOT_SUPPORTED);
    fake_weight_port_set_cal_status(WEIGHT_CAL_CAPTURING_SPAN);
    TEST_ASSERT_TRUE(weigh_cli_print_read_fail(PORT_ERR_NOT_SUPPORTED));
}
