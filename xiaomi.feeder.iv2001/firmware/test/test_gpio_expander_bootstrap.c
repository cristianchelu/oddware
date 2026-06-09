/* Tests: spec/30-processes/display-driver.md § Boot self-test step 2 */

#include "unity.h"

#include "aw9523b.h"
#include "board_gpio_iv2001.h"
#include "fake_i2c_bus.h"
#include "gpio_expander_bootstrap.h"
#include "fake_gpio_expander_port.h"

void test_gpio_expander_bootstrap_golden_i2c_trace(void)
{
    const fake_i2c_op_t *ops;
    size_t count;
    bool found_dir_p0 = false;

    bool found_ctl_write = false;

    fake_gpio_expander_reset();
    TEST_ASSERT_EQUAL(PORT_OK, gpio_expander_bootstrap(fake_gpio_expander_port_get()));

    ops = fake_i2c_bus_ops(&count);
    TEST_ASSERT_TRUE(count >= 2u);

    for (size_t i = 0; i < count; i++) {
        if (ops[i].is_write && ops[i].reg == AW9523B_REG_DIR_P0) {
            found_dir_p0 = true;
            TEST_ASSERT_EQUAL(BOARD_GPIO_BOOT_DIR_P0, ops[i].val);
        }
        if (ops[i].is_write && ops[i].reg == AW9523B_REG_OUTPUT_P0) {
            TEST_ASSERT_EQUAL(0u, ops[i].val & BOARD_GPIO_MOTOR_EN_MASK);
            TEST_ASSERT_EQUAL(0u, ops[i].val & BOARD_GPIO_MOTOR_PH_MASK);
            TEST_ASSERT_EQUAL(0u, ops[i].val & BOARD_GPIO_DISPLAY_RAIL_MASK);
        }
        if (ops[i].is_write && ops[i].reg == AW9523B_REG_CTL) {
            found_ctl_write = true;
            TEST_ASSERT_NOT_EQUAL(0u, ops[i].val & AW9523B_CTL_GPIO_MODE);
        }
    }

    TEST_ASSERT_TRUE(found_dir_p0);
    TEST_ASSERT_TRUE(found_ctl_write);
}

void test_gpio_expander_bootstrap_propagates_reset_failure(void)
{
    fake_gpio_expander_reset();
    fake_gpio_expander_set_reset_err(PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, gpio_expander_bootstrap(fake_gpio_expander_port_get()));
}

void test_gpio_expander_bootstrap_rejects_wrong_id(void)
{
    fake_gpio_expander_reset();
    fake_gpio_expander_set_id(0x00u);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, gpio_expander_bootstrap(fake_gpio_expander_port_get()));
}
