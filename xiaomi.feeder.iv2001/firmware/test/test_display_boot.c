/* Tests: spec/30-processes/display-presentation.md § Boot self-test */

#include "unity.h"

#include "display_boot.h"
#include "fake_display_port.h"

static uint32_t s_delay_log[4];
static size_t s_delay_count;

void display_boot_delay_ms(uint32_t ms)
{
    if (s_delay_count < (sizeof(s_delay_log) / sizeof(s_delay_log[0]))) {
        s_delay_log[s_delay_count++] = ms;
    }
}

void test_display_boot_run_sequence(void)
{
    const fake_display_op_t *ops;
    size_t count;

    fake_display_port_reset();
    s_delay_count = 0u;

    TEST_ASSERT_EQUAL(PORT_OK, display_boot_run());

    ops = fake_display_port_ops(&count);
    TEST_ASSERT_EQUAL(4, count);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_POWER_ON, ops[0].kind);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_SHOW_FILL, ops[1].kind);
    TEST_ASSERT_EQUAL(0xFFu, ops[1].segment_byte);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_BLANK, ops[2].kind);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_POWER_OFF, ops[3].kind);

    TEST_ASSERT_EQUAL(2, s_delay_count);
    TEST_ASSERT_EQUAL(DISPLAY_BOOT_PRE_POWER_MS, s_delay_log[0]);
    TEST_ASSERT_EQUAL(DISPLAY_BOOT_LIGHT_TEST_MS, s_delay_log[1]);
}

void test_display_boot_run_stops_on_power_on_failure(void)
{
    size_t count;

    fake_display_port_reset();
    fake_display_port_set_power_on_err(PORT_ERR_IO);
    s_delay_count = 0u;

    TEST_ASSERT_EQUAL(PORT_ERR_IO, display_boot_run());

    (void)fake_display_port_ops(&count);
    TEST_ASSERT_EQUAL(0, count);
}

void test_display_boot_run_stops_on_show_fill_failure(void)
{
    const fake_display_op_t *ops;
    size_t count;

    fake_display_port_reset();
    fake_display_port_set_show_fill_err(PORT_ERR_IO);
    s_delay_count = 0u;

    TEST_ASSERT_EQUAL(PORT_ERR_IO, display_boot_run());

    ops = fake_display_port_ops(&count);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_POWER_ON, ops[0].kind);
}
