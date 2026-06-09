/* Tests: spec/10-hardware/pinmap.md — I2C1 pin roles vs board_gpio_iv2001.h */

#include "unity.h"

#include "board_gpio_iv2001.h"

void test_board_gpio_i2c_pins_match_pinmap(void)
{
    TEST_ASSERT_EQUAL(15u, BOARD_GPIO_I2C_SCL_PIN);
    TEST_ASSERT_EQUAL(16u, BOARD_GPIO_I2C_SDA_PIN);
}

void test_board_gpio_boot_rail_starts_off(void)
{
    TEST_ASSERT_EQUAL(0u, BOARD_GPIO_BOOT_OUT_P0 & BOARD_GPIO_DISPLAY_RAIL_MASK);
}
