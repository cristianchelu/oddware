/* Tests: spec/30-processes/uart-console.md § display commands */

#include "unity.h"

#include "display_cli.h"
#include "display_boot.h"
#include "fake_display_port.h"

static uint32_t s_delay_ms;

void display_cli_delay_ms(uint32_t ms)
{
    s_delay_ms += ms;
}

void test_display_cli_parse_hex_byte_accepts_one_and_two_digits(void)
{
    uint8_t val;

    TEST_ASSERT_EQUAL(PORT_OK, display_cli_parse_hex_byte("FF", &val));
    TEST_ASSERT_EQUAL_HEX8(0xFFu, val);

    TEST_ASSERT_EQUAL(PORT_OK, display_cli_parse_hex_byte("a", &val));
    TEST_ASSERT_EQUAL_HEX8(0x0Au, val);
}

void test_display_cli_parse_hex_byte_rejects_invalid(void)
{
    uint8_t val;

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, display_cli_parse_hex_byte("", &val));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, display_cli_parse_hex_byte("gg", &val));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, display_cli_parse_hex_byte("100", &val));
}

void test_display_cli_run_test_full_sequence(void)
{
    const fake_display_op_t *ops;
    size_t count;

    fake_display_port_reset();
    s_delay_ms = 0u;

    TEST_ASSERT_EQUAL(PORT_OK, display_cli_run_test());

    ops = fake_display_port_ops(&count);
    TEST_ASSERT_EQUAL(4u, count);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_POWER_ON, ops[0].kind);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_SHOW_FILL, ops[1].kind);
    TEST_ASSERT_EQUAL_HEX8(0xFFu, ops[1].segment_byte);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_BLANK, ops[2].kind);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_POWER_OFF, ops[3].kind);
    TEST_ASSERT_EQUAL(DISPLAY_BOOT_LIGHT_TEST_MS, s_delay_ms);
}

void test_display_cli_run_fill_powers_on_and_shows(void)
{
    const fake_display_op_t *ops;
    size_t count;

    fake_display_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, display_cli_run_fill(0x7Eu));

    ops = fake_display_port_ops(&count);
    TEST_ASSERT_EQUAL(2u, count);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_POWER_ON, ops[0].kind);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_SHOW_FILL, ops[1].kind);
    TEST_ASSERT_EQUAL_HEX8(0x7Eu, ops[1].segment_byte);
}

void test_display_cli_run_off_calls_power_off(void)
{
    const fake_display_op_t *ops;
    size_t count;

    fake_display_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, display_cli_run_off());

    ops = fake_display_port_ops(&count);
    TEST_ASSERT_EQUAL(1u, count);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_POWER_OFF, ops[0].kind);
}
