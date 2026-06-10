/* Tests: spec/30-processes/uart-console.md § index / hopper read */

#include "unity.h"

#include "beam_cli.h"
#include "fake_hopper_ir_port.h"
#include "fake_motor_index_port.h"

void test_beam_cli_index_read_turns_led_on_then_off(void)
{
    bool beam_open = false;

    fake_motor_index_port_reset();
    fake_motor_index_port_set_beam_open(true);

    TEST_ASSERT_EQUAL(PORT_OK, beam_cli_run_index_read(&beam_open));
    TEST_ASSERT_TRUE(beam_open);
    TEST_ASSERT_EQUAL(2u, fake_motor_index_port_set_led_calls());
    TEST_ASSERT_FALSE(fake_motor_index_port_get_led());
}

void test_beam_cli_index_read_propagates_detector_error(void)
{
    bool beam_open = false;

    fake_motor_index_port_reset();
    fake_motor_index_port_set_read_err(PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, beam_cli_run_index_read(&beam_open));
    TEST_ASSERT_FALSE(fake_motor_index_port_get_led());
}

void test_beam_cli_hopper_read_uses_port(void)
{
    bool blocked = false;

    fake_hopper_ir_port_reset();
    fake_hopper_ir_port_set_beam_blocked(true);

    TEST_ASSERT_EQUAL(PORT_OK, beam_cli_run_hopper_read(&blocked));
    TEST_ASSERT_TRUE(blocked);
}

void test_beam_cli_hopper_read_propagates_error(void)
{
    bool blocked = false;

    fake_hopper_ir_port_reset();
    fake_hopper_ir_port_set_sense_err(PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, beam_cli_run_hopper_read(&blocked));
}
