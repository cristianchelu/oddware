/* Tests: spec/30-processes/display-driver.md § Boot self-test — driver stack */

#include "unity.h"

#include "board_gpio_iv2001.h"
#include "display_boot.h"
#include "display_driver.h"
#include "fake_gpio_expander_port.h"
#include "fake_tm1637_gpio.h"

static uint32_t s_stack_delay_ms;

static void stack_delay_ms(uint32_t ms)
{
    s_stack_delay_ms += ms;
}

void test_display_boot_stack_full_sequence(void)
{
    display_driver_state_t state;
    display_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .tm1637_gpio = fake_tm1637_gpio_ops_get(),
        .prepare_tm1637_pins = NULL,
        .delay_ms = stack_delay_ms,
    };
    size_t tm1637_count;

    fake_gpio_expander_reset();
    fake_tm1637_gpio_reset();
    s_stack_delay_ms = 0u;

    display_driver_init(&state, &hw);

    display_boot_delay_ms(DISPLAY_BOOT_PRE_POWER_MS);
    TEST_ASSERT_EQUAL(PORT_OK, display_power_on(&state));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                            BOARD_GPIO_DISPLAY_RAIL_PIN));

    TEST_ASSERT_EQUAL(PORT_OK, display_show_fill(&state, 0xFFu));
    display_boot_delay_ms(DISPLAY_BOOT_LIGHT_TEST_MS);
    TEST_ASSERT_EQUAL(PORT_OK, display_blank(&state));
    TEST_ASSERT_EQUAL(PORT_OK, display_power_off(&state));

    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                             BOARD_GPIO_DISPLAY_RAIL_PIN));
    TEST_ASSERT_GREATER_OR_EQUAL(DISPLAY_RAIL_SETTLE_MS, s_stack_delay_ms);
    (void)fake_tm1637_gpio_bytes(&tm1637_count);
    TEST_ASSERT_TRUE(tm1637_count > 0u);
}
