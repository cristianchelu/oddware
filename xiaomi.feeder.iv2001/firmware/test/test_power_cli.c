/* Tests: spec/30-processes/uart-console.md § power commands */

#include <string.h>

#include "unity.h"

#include "app_power_cli.h"
#include "cli_test_assert.h"
#include "fake_power_source_port.h"
#include "power_cli.h"
#include "power_source_input.h"

static uint8_t run_power_cli_show(void)
{
    size_t i;

    for (i = 0u; power_cli_subcmds[i].cmd != NULL; i++) {
        if (strcmp(power_cli_subcmds[i].cmd, "show") == 0) {
            return power_cli_subcmds[i].fn(0, NULL);
        }
    }

    return 0xFFu;
}

void test_power_cli_format_source_mains(void)
{
    TEST_ASSERT_EQUAL_STRING("mains", power_cli_format_source(POWER_SOURCE_MAINS));
}

void test_power_cli_format_source_battery(void)
{
    TEST_ASSERT_EQUAL_STRING("battery", power_cli_format_source(POWER_SOURCE_BATTERY));
}

void test_power_cli_show_mains_success(void)
{
    power_source_t source = POWER_SOURCE_BATTERY;

    fake_power_source_port_reset();
    fake_power_source_port_set_mains_present(true);
    power_source_input_init(fake_power_source_port_get());

    TEST_ASSERT_EQUAL(PORT_OK, power_cli_run_show(&source));
    TEST_ASSERT_EQUAL(POWER_SOURCE_MAINS, source);
}

void test_power_cli_show_battery_success(void)
{
    power_source_t source = POWER_SOURCE_MAINS;

    fake_power_source_port_reset();
    fake_power_source_port_set_mains_present(false);
    power_source_input_init(fake_power_source_port_get());

    TEST_ASSERT_EQUAL(PORT_OK, power_cli_run_show(&source));
    TEST_ASSERT_EQUAL(POWER_SOURCE_BATTERY, source);
}

void test_power_cli_show_fails_when_state_invalid(void)
{
    power_source_t source = POWER_SOURCE_MAINS;

    fake_power_source_port_reset();
    fake_power_source_port_set_read_err(PORT_ERR_IO);
    power_source_input_init(fake_power_source_port_get());

    TEST_ASSERT_EQUAL(PORT_ERR_IO, power_cli_run_show(&source));
}

void test_power_cli_print_fail_io(void)
{
    cli_test_reset();
    power_cli_print_fail(PORT_ERR_IO);
    assert_cli_body("power show failed (io)");
}

void test_power_cli_run_show_rejects_null(void)
{
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, power_cli_run_show(NULL));
}

void test_app_power_cli_show_mains(void)
{
    fake_power_source_port_reset();
    fake_power_source_port_set_mains_present(true);
    power_source_input_init(fake_power_source_port_get());

    cli_test_reset();
    TEST_ASSERT_EQUAL(0, run_power_cli_show());
    assert_cli_body("power source: mains");
}

void test_app_power_cli_show_battery(void)
{
    fake_power_source_port_reset();
    fake_power_source_port_set_mains_present(false);
    power_source_input_init(fake_power_source_port_get());

    cli_test_reset();
    TEST_ASSERT_EQUAL(0, run_power_cli_show());
    assert_cli_body("power source: battery");
}

void test_app_power_cli_show_fails_when_invalid(void)
{
    fake_power_source_port_reset();
    fake_power_source_port_set_read_err(PORT_ERR_IO);
    power_source_input_init(fake_power_source_port_get());

    cli_test_reset();
    TEST_ASSERT_EQUAL(1, run_power_cli_show());
    assert_cli_body("power show failed (io)");
}
