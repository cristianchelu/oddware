/* Tests: spec/30-processes/display-driver.md § Rail power primitives */

#include "unity.h"

#include "board_gpio_iv2001.h"
#include "display_rail.h"
#include "fake_gpio_expander_port.h"
#include "fake_time.h"

static void test_delay_ms(uint32_t ms)
{
    fake_time_advance_ms(ms);
}

void test_display_rail_on_sets_p05_and_settles(void)
{
    display_rail_ctx_t ctx;

    fake_gpio_expander_reset();
    fake_time_reset();
    display_rail_ctx_init(&ctx, fake_gpio_expander_port_get(), test_delay_ms);

    TEST_ASSERT_FALSE(display_rail_is_settled(&ctx));
    TEST_ASSERT_EQUAL(PORT_OK, display_rail_on(&ctx));
    TEST_ASSERT_TRUE(display_rail_is_settled(&ctx));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                            BOARD_GPIO_DISPLAY_RAIL_PIN));
}

void test_display_rail_off_clears_settled(void)
{
    display_rail_ctx_t ctx;

    fake_gpio_expander_reset();
    display_rail_ctx_init(&ctx, fake_gpio_expander_port_get(), test_delay_ms);
    TEST_ASSERT_EQUAL(PORT_OK, display_rail_on(&ctx));
    TEST_ASSERT_EQUAL(PORT_OK, display_rail_off(&ctx));
    TEST_ASSERT_FALSE(display_rail_is_settled(&ctx));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                             BOARD_GPIO_DISPLAY_RAIL_PIN));
}
