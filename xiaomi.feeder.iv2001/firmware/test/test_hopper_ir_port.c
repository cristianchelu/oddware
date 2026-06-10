/* Tests: spec/40-architecture/ports.md — hopper_ir_port fake wiring */

#include "unity.h"

#include "fake_hopper_ir_port.h"
#include "hopper_ir_port.h"

void test_hopper_ir_port_sense_blocked(void)
{
    bool blocked = false;

    fake_hopper_ir_port_reset();
    fake_hopper_ir_port_set_beam_blocked(true);
    TEST_ASSERT_EQUAL(PORT_OK, hopper_ir_port_get()->sense(&blocked));
    TEST_ASSERT_TRUE(blocked);
}

void test_hopper_ir_port_sense_clear(void)
{
    bool blocked = true;

    fake_hopper_ir_port_reset();
    fake_hopper_ir_port_set_beam_blocked(false);
    TEST_ASSERT_EQUAL(PORT_OK, hopper_ir_port_get()->sense(&blocked));
    TEST_ASSERT_FALSE(blocked);
}

void test_hopper_ir_port_sense_null_arg(void)
{
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, hopper_ir_port_get()->sense(NULL));
}

void test_hopper_ir_port_sense_io_error(void)
{
    bool blocked = false;

    fake_hopper_ir_port_reset();
    fake_hopper_ir_port_set_sense_err(PORT_ERR_IO);
    TEST_ASSERT_EQUAL(PORT_ERR_IO, hopper_ir_port_get()->sense(&blocked));
}
