/* Tests: spec/40-architecture/ports.md § gpio_expander_port.get_pin */

#include "unity.h"

#include "board_gpio_iv2001.h"
#include "fake_gpio_expander_port.h"

/* Regression: output pins return commanded level from output latch, not input pad. */
void test_gpio_expander_get_pin_reads_output_latch(void)
{
    const gpio_expander_port_t *exp = fake_gpio_expander_port_get();
    bool level;

    fake_gpio_expander_reset();
    TEST_ASSERT_EQUAL(PORT_OK, exp->reset());
    TEST_ASSERT_EQUAL(PORT_OK,
                      exp->configure(BOARD_GPIO_BOOT_DIR_P0,
                                     BOARD_GPIO_BOOT_DIR_P1,
                                     BOARD_GPIO_MOTOR_EN_MASK,
                                     BOARD_GPIO_BOOT_OUT_P1));
    fake_gpio_expander_set_inputs(0u, 0u);

    TEST_ASSERT_EQUAL(PORT_OK,
                      exp->get_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                   BOARD_GPIO_MOTOR_EN_PIN,
                                   &level));
    TEST_ASSERT_TRUE(level);
}

void test_gpio_expander_get_pin_reads_input_pad_for_inputs(void)
{
    const gpio_expander_port_t *exp = fake_gpio_expander_port_get();
    bool level;

    fake_gpio_expander_reset();
    TEST_ASSERT_EQUAL(PORT_OK, exp->reset());
    TEST_ASSERT_EQUAL(PORT_OK,
                      exp->configure(BOARD_GPIO_BOOT_DIR_P0,
                                     BOARD_GPIO_BOOT_DIR_P1,
                                     BOARD_GPIO_BOOT_OUT_P0,
                                     BOARD_GPIO_BOOT_OUT_P1));
    fake_gpio_expander_set_inputs(BOARD_GPIO_BTN_POWER_MASK, 0u);

    TEST_ASSERT_EQUAL(PORT_OK, exp->get_pin(0u, 3u, &level));
    TEST_ASSERT_TRUE(level);
}
