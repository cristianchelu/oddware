/* Tests: spec/30-processes/uart-console.md § adc commands */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "adc_cli.h"
#include "adc_port.h"
#include "config_keys.h"
#include "fake_adc_port.h"
#include "fake_config_port.h"

static void assert_adc_cli_fail_msg(port_err_t err, const char *expect)
{
    char buf[96];
    FILE *saved = stdout;
    FILE *cap = tmpfile();

    TEST_ASSERT_NOT_NULL(cap);
    stdout = cap;
    adc_cli_print_fail(err);
    fflush(stdout);
    rewind(cap);
    TEST_ASSERT_NOT_NULL(fgets(buf, sizeof(buf), cap));
    stdout = saved;
    fclose(cap);
    TEST_ASSERT_EQUAL_STRING(expect, buf);
}

void test_adc_cli_motor_read_success(void)
{
    uint16_t ma = 0u;

    fake_adc_port_reset();
    fake_adc_port_set_motor_ma(1250u);

    TEST_ASSERT_EQUAL(PORT_OK, adc_cli_run_motor_read(&ma));
    TEST_ASSERT_EQUAL_UINT16(1250u, ma);
}

void test_adc_cli_battery_read_success(void)
{
    uint16_t mv = 0u;

    fake_adc_port_reset();
    fake_adc_port_set_battery_mv(3300u);

    TEST_ASSERT_EQUAL(PORT_OK, adc_cli_run_battery_read(&mv));
    TEST_ASSERT_EQUAL_UINT16(3300u, mv);
}

void test_adc_cli_motor_read_propagates_port_error(void)
{
    uint16_t ma = 0u;

    fake_adc_port_reset();
    fake_adc_port_set_motor_err(PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, adc_cli_run_motor_read(&ma));
}

void test_adc_cli_battery_read_propagates_busy(void)
{
    uint16_t mv = 0u;

    fake_adc_port_reset();
    fake_adc_port_set_battery_err(PORT_ERR_BUSY);

    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, adc_cli_run_battery_read(&mv));
}

void test_adc_cli_print_fail_busy(void)
{
    assert_adc_cli_fail_msg(PORT_ERR_BUSY, "adc read failed (busy)\r\n");
}

void test_adc_cli_print_fail_io(void)
{
    assert_adc_cli_fail_msg(PORT_ERR_IO, "adc read failed (io)\r\n");
}

void test_adc_cli_run_rejects_null_mv(void)
{
    fake_adc_port_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_IO, adc_cli_run_motor_read(NULL));
    TEST_ASSERT_EQUAL(PORT_ERR_IO, adc_cli_run_battery_read(NULL));
}

void test_adc_cli_cal_capture_stores_ratio(void)
{
    adc_cal_status_t status;

    fake_config_port_reset();
    fake_adc_port_reset();
    fake_adc_port_set_battery_pin_mv(600u);

    TEST_ASSERT_EQUAL(PORT_OK, adc_cli_run_cal_capture(6385u));
    TEST_ASSERT_EQUAL(PORT_OK, adc_cli_run_cal_status(&status));
    TEST_ASSERT_TRUE(status.customized);
    TEST_ASSERT_EQUAL_UINT32(10642u, status.scale_x1000);
}

void test_adc_cli_format_cal_status_default(void)
{
    adc_cal_status_t status = {
        .scale_x1000 = 11000u,
        .customized = false,
    };
    char buf[32];

    adc_cli_format_cal_status(&status, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("11.000 (default)", buf);
}

void test_adc_cli_cal_reset_restores_default(void)
{
    adc_cal_status_t status;

    fake_config_port_reset();
    fake_adc_port_reset();
    fake_adc_port_set_battery_pin_mv(600u);
    TEST_ASSERT_EQUAL(PORT_OK, adc_cli_run_cal_capture(6385u));

    TEST_ASSERT_EQUAL(PORT_OK, adc_cli_run_cal_reset());
    TEST_ASSERT_EQUAL(PORT_OK, adc_cli_run_cal_status(&status));
    TEST_ASSERT_FALSE(status.customized);
    TEST_ASSERT_EQUAL_UINT32(11000u, status.scale_x1000);
}

void test_adc_cli_cal_capture_rejects_when_motor_busy(void)
{
    fake_adc_port_reset();
    fake_adc_port_set_battery_err(PORT_ERR_BUSY);

    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, adc_cli_run_cal_capture(6385u));
}
