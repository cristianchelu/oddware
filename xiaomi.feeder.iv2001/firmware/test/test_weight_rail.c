/* Tests: spec/30-processes/weighing.md § CS1270 lifecycle */

#include "unity.h"

#include "board_gpio_iv2001.h"
#include "cs1270.h"
#include "fake_gpio_expander_port.h"
#include "fake_time.h"
#include "weight_driver.h"
#include "weight_rail.h"

static void test_delay_ms(uint32_t ms)
{
    fake_time_advance_ms(ms);
}

static port_err_t fake_uart_exchange(const uint8_t tx[CS1270_FRAME_LEN],
                                     uint8_t rx[CS1270_FRAME_LEN],
                                     uint32_t timeout_ms)
{
    (void)tx;
    (void)timeout_ms;

    rx[0] = 0xB2u;
    rx[1] = 0xA5u;
    rx[2] = 0x00u;
    rx[3] = 0x00u;
    rx[4] = 0x00u;
    rx[5] = cs1270_checksum_rsp(0x00u, 0x00u, 0x00u);
    return PORT_OK;
}

static const cs1270_uart_ops_t s_uart_ops = {
    .exchange = fake_uart_exchange,
    .delay_ms = test_delay_ms,
};

void test_weight_read_after_power_off_returns_not_found(void)
{
    weight_driver_state_t state;
    weight_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .uart = &s_uart_ops,
        .delay_ms = test_delay_ms,
    };
    int32_t grams = -1;

    fake_gpio_expander_reset();
    weight_driver_init(&state, &hw);
    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_enable(&state));
    TEST_ASSERT_EQUAL(PORT_OK, weight_boot_settle(&state));
    TEST_ASSERT_EQUAL(PORT_OK, weight_power_off(&state));
    TEST_ASSERT_TRUE(weight_scale_off(&state));
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_FOUND, weight_read_grams(&state, &grams));
}

void test_weight_rail_on_sets_p02_and_settles(void)
{
    weight_rail_ctx_t ctx;

    fake_gpio_expander_reset();
    fake_time_reset();
    weight_rail_ctx_init(&ctx, fake_gpio_expander_port_get(), test_delay_ms);

    TEST_ASSERT_FALSE(weight_rail_is_settled(&ctx));
    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_on(&ctx));
    TEST_ASSERT_TRUE(weight_rail_is_settled(&ctx));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_CS1270_PWR_PORT,
                                            BOARD_GPIO_CS1270_PWR_PIN));
}

/*
 * Regression: weigh power on must clear scale_off (adapter boot_sequence, not
 * ensure_booted). Driver rail+boot is the state transition power on performs.
 */
void test_weight_rail_enable_after_power_off(void)
{
    weight_driver_state_t state;
    weight_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .uart = &s_uart_ops,
        .delay_ms = test_delay_ms,
    };
    int32_t grams = -1;

    fake_gpio_expander_reset();
    weight_driver_init(&state, &hw);
    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_enable(&state));
    TEST_ASSERT_EQUAL(PORT_OK, weight_boot_settle(&state));
    TEST_ASSERT_EQUAL(PORT_OK, weight_read_raw_grams(&state, &grams));

    TEST_ASSERT_EQUAL(PORT_OK, weight_power_off(&state));
    TEST_ASSERT_TRUE(weight_scale_off(&state));
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_FOUND, weight_read_grams(&state, &grams));

    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_enable(&state));
    TEST_ASSERT_FALSE(weight_scale_off(&state));
    TEST_ASSERT_EQUAL(PORT_OK, weight_boot_settle(&state));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_CS1270_PWR_PORT,
                                            BOARD_GPIO_CS1270_PWR_PIN));
    TEST_ASSERT_EQUAL(PORT_OK, weight_read_raw_grams(&state, &grams));
}

void test_weight_rail_off_clears_settled(void)
{
    weight_rail_ctx_t ctx;

    fake_gpio_expander_reset();
    weight_rail_ctx_init(&ctx, fake_gpio_expander_port_get(), test_delay_ms);
    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_on(&ctx));
    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_off(&ctx));
    TEST_ASSERT_FALSE(weight_rail_is_settled(&ctx));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_CS1270_PWR_PORT,
                                             BOARD_GPIO_CS1270_PWR_PIN));
}
