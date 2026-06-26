/* Tests: spec/30-processes/uart-console.md § weigh commands */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "app_weigh_cli.h"
#include "cli_test_assert.h"
#include "fake_weight_port.h"
#include "weigh_cli.h"
#include "weight_units.h"

static void weigh_cli_test_reset(void)
{
    fake_weight_port_reset();
    cli_test_reset();
}

static uint8_t weigh_cli_test_run_read(void)
{
    size_t i;

    for (i = 0u; weigh_cli_subcmds[i].cmd != NULL; i++) {
        if (strcmp(weigh_cli_subcmds[i].cmd, "read") == 0) {
            return weigh_cli_subcmds[i].fn(0, NULL);
        }
    }

    TEST_FAIL_MESSAGE("weigh read subcmd missing");
    return 1;
}

void test_weigh_cli_run_read_uses_port(void)
{
    const fake_weight_op_t *ops;
    size_t count;
    weight_dg_t dg;

    fake_weight_port_reset();
    fake_weight_port_set_read_dg(2500);

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cli_run_read(&dg));
    TEST_ASSERT_EQUAL(2500, dg);

    ops = fake_weight_port_ops(&count);
    TEST_ASSERT_EQUAL(1u, count);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_READ_DG, ops[0].kind);
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
    weight_dg_t dg;

    fake_weight_port_reset();
    fake_weight_port_set_scale_off(true);
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_FOUND, weigh_cli_run_read(&dg));
}

void test_weigh_cli_power_on_after_power_off(void)
{
    const fake_weight_op_t *ops;
    size_t count;
    weight_dg_t dg;

    fake_weight_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cli_run_power_off());
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_FOUND, weigh_cli_run_read(&dg));
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cli_run_power_on());
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cli_run_read(&dg));

    ops = fake_weight_port_ops(&count);
    TEST_ASSERT_EQUAL(3u, count);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_POWER_OFF, ops[0].kind);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_POWER_ON, ops[1].kind);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_READ_DG, ops[2].kind);
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

void test_weigh_cli_read_prints_one_decimal_grams(void)
{
    weigh_cli_test_reset();
    fake_weight_port_set_read_dg(423);

    TEST_ASSERT_EQUAL_UINT8(0, weigh_cli_test_run_read());
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(0), "weight: 42.3 g"));
}
