/* Tests: spec/40-architecture/ports.md — motor_index_port */

#include "unity.h"

#include "fake_motor_index_port.h"
#include "motor_index_port.h"
#include "motor_index_port_provider_host.h"

static void motor_index_port_test_setup(void)
{
    motor_index_port_host_reset();
    fake_motor_index_port_reset();
}

void test_motor_index_port_sense_illuminates_and_deilluminates(void)
{
    bool beam_open = false;

    motor_index_port_test_setup();
    fake_motor_index_port_set_beam_open(true);

    TEST_ASSERT_EQUAL(PORT_OK, motor_index_port_get()->sense(&beam_open));
    TEST_ASSERT_TRUE(beam_open);
    TEST_ASSERT_EQUAL(2u, fake_motor_index_port_set_led_calls());
    TEST_ASSERT_FALSE(fake_motor_index_port_get_led());
    TEST_ASSERT_FALSE(fake_motor_index_port_session_active());
}

void test_motor_index_port_sense_blocked_when_led_would_be_off(void)
{
    bool beam_open = true;

    motor_index_port_test_setup();
    fake_motor_index_port_set_beam_open(false);

    TEST_ASSERT_EQUAL(PORT_OK, motor_index_port_get()->sense(&beam_open));
    TEST_ASSERT_FALSE(beam_open);
}

void test_motor_index_port_poll_requires_session(void)
{
    bool beam_open = false;

    motor_index_port_test_setup();
    fake_motor_index_port_set_beam_open(true);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, motor_index_port_get()->poll(&beam_open));
}

void test_motor_index_port_session_poll_while_active(void)
{
    bool beam_open = false;

    motor_index_port_test_setup();
    fake_motor_index_port_set_beam_open(true);

    TEST_ASSERT_EQUAL(PORT_OK, motor_index_port_get()->session_begin());
    TEST_ASSERT_TRUE(fake_motor_index_port_session_active());
    TEST_ASSERT_EQUAL(PORT_OK, motor_index_port_get()->poll(&beam_open));
    TEST_ASSERT_TRUE(beam_open);
    TEST_ASSERT_EQUAL(PORT_OK, motor_index_port_get()->session_end());
    TEST_ASSERT_FALSE(fake_motor_index_port_get_led());
}

void test_motor_index_port_sense_propagates_read_error(void)
{
    bool beam_open = false;

    motor_index_port_test_setup();
    fake_motor_index_port_set_read_err(PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, motor_index_port_get()->sense(&beam_open));
    TEST_ASSERT_FALSE(fake_motor_index_port_get_led());
}
