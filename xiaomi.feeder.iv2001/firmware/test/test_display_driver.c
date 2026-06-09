/* Tests: spec/30-processes/display-driver.md § Software layering */

#include "unity.h"

#include "board_gpio_iv2001.h"
#include "display_driver.h"
#include "fake_gpio_expander_port.h"
#include "fake_tm1637_gpio.h"

static uint32_t s_delay_total_ms;
static bool s_tm1637_before_settle;

static void test_delay_ms(uint32_t ms)
{
    if (s_delay_total_ms < DISPLAY_RAIL_SETTLE_MS) {
        size_t count = 0u;
        (void)fake_tm1637_gpio_bytes(&count);
        if (count > 0u) {
            s_tm1637_before_settle = true;
        }
    }
    s_delay_total_ms += ms;
}

void test_display_power_on_no_tm1637_before_rail_settle(void)
{
    display_driver_state_t state;
    display_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .tm1637_gpio = fake_tm1637_gpio_ops_get(),
        .delay_ms = test_delay_ms,
    };

    fake_gpio_expander_reset();
    fake_tm1637_gpio_reset();
    s_delay_total_ms = 0u;
    s_tm1637_before_settle = false;

    display_driver_init(&state, &hw);
    TEST_ASSERT_EQUAL(PORT_OK, display_power_on(&state));
    TEST_ASSERT_FALSE(s_tm1637_before_settle);
    TEST_ASSERT_GREATER_OR_EQUAL(DISPLAY_RAIL_SETTLE_MS, s_delay_total_ms);
}

void test_display_show_fill_rejects_before_power_on(void)
{
    display_driver_state_t state;
    display_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .tm1637_gpio = fake_tm1637_gpio_ops_get(),
        .delay_ms = test_delay_ms,
    };

    fake_gpio_expander_reset();
    display_driver_init(&state, &hw);
    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, display_show_fill(&state, 0xFFu));
}

void test_display_show_fill_after_power_on(void)
{
    display_driver_state_t state;
    display_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .tm1637_gpio = fake_tm1637_gpio_ops_get(),
        .delay_ms = test_delay_ms,
    };
    size_t count;

    fake_gpio_expander_reset();
    fake_tm1637_gpio_reset();
    display_driver_init(&state, &hw);
    TEST_ASSERT_EQUAL(PORT_OK, display_power_on(&state));

    fake_tm1637_gpio_reset();
    TEST_ASSERT_EQUAL(PORT_OK, display_show_fill(&state, 0xFFu));
    (void)fake_tm1637_gpio_bytes(&count);
    TEST_ASSERT_TRUE(count > 0u);
}

void test_display_power_on_propagates_bootstrap_failure(void)
{
    display_driver_state_t state;
    display_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .tm1637_gpio = fake_tm1637_gpio_ops_get(),
        .delay_ms = test_delay_ms,
    };

    fake_gpio_expander_reset();
    fake_gpio_expander_set_reset_err(PORT_ERR_IO);
    display_driver_init(&state, &hw);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, display_power_on(&state));
    TEST_ASSERT_FALSE(state.powered);
}

void test_display_show_grids_rejects_before_power_on(void)
{
    display_driver_state_t state;
    display_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .tm1637_gpio = fake_tm1637_gpio_ops_get(),
        .delay_ms = test_delay_ms,
    };
    const uint8_t grids[TM1637_GRID_COUNT] = {0x3Fu, 0x06u, 0x5Bu, 0x01u, 0x02u};

    fake_gpio_expander_reset();
    display_driver_init(&state, &hw);
    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, display_show_grids(&state, grids));
}

void test_display_show_grids_after_power_on(void)
{
    display_driver_state_t state;
    display_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .tm1637_gpio = fake_tm1637_gpio_ops_get(),
        .delay_ms = test_delay_ms,
    };
    const uint8_t grids[TM1637_GRID_COUNT] = {0x3Fu, 0x06u, 0x5Bu, 0x01u, 0x02u};
    size_t count;

    fake_gpio_expander_reset();
    fake_tm1637_gpio_reset();
    display_driver_init(&state, &hw);
    TEST_ASSERT_EQUAL(PORT_OK, display_power_on(&state));

    fake_tm1637_gpio_reset();
    TEST_ASSERT_EQUAL(PORT_OK, display_show_grids(&state, grids));
    (void)fake_tm1637_gpio_bytes(&count);
    TEST_ASSERT_TRUE(count > 0u);
}

void test_display_power_off_clears_rail(void)
{
    display_driver_state_t state;
    display_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .tm1637_gpio = fake_tm1637_gpio_ops_get(),
        .delay_ms = test_delay_ms,
    };

    fake_gpio_expander_reset();
    display_driver_init(&state, &hw);
    TEST_ASSERT_EQUAL(PORT_OK, display_power_on(&state));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                            BOARD_GPIO_DISPLAY_RAIL_PIN));

    TEST_ASSERT_EQUAL(PORT_OK, display_power_off(&state));
    TEST_ASSERT_FALSE(state.powered);
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                             BOARD_GPIO_DISPLAY_RAIL_PIN));
}
