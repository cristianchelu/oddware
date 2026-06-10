/* Behavioral tests: display and weigh share AW9523B without clobbering each other */

#include "unity.h"

#include "board_gpio_iv2001.h"
#include "cs1270.h"
#include "display_driver.h"
#include "fake_config_port.h"
#include "fake_gpio_expander_port.h"
#include "fake_time.h"
#include "fake_tm1637_gpio.h"
#include "gpio_expander_bootstrap.h"
#include "weight_driver.h"

static uint32_t s_disp_delay_ms;

static void disp_delay_ms(uint32_t ms)
{
    s_disp_delay_ms += ms;
}

static void weigh_delay_ms(uint32_t ms)
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
    .delay_ms = weigh_delay_ms,
};

static void stack_init(display_driver_state_t *disp, weight_driver_state_t *weigh)
{
    const gpio_expander_port_t *exp = fake_gpio_expander_port_get();
    display_hw_t disp_hw = {
        .expander = exp,
        .tm1637_gpio = fake_tm1637_gpio_ops_get(),
        .delay_ms = disp_delay_ms,
    };
    weight_hw_t weigh_hw = {
        .expander = exp,
        .uart = &s_uart_ops,
        .config = fake_config_port_get(),
        .delay_ms = weigh_delay_ms,
    };

    fake_gpio_expander_reset();
    fake_tm1637_gpio_reset();
    fake_time_reset();
    s_disp_delay_ms = 0u;

    display_driver_init(disp, &disp_hw);
    weight_driver_init(weigh, &weigh_hw);
}

static bool display_rail_is_high(void)
{
    return fake_gpio_expander_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                  BOARD_GPIO_DISPLAY_RAIL_PIN);
}

static bool weigh_rail_is_high(void)
{
    return fake_gpio_expander_pin(BOARD_GPIO_CS1270_PWR_PORT,
                                  BOARD_GPIO_CS1270_PWR_PIN);
}

void test_display_stays_on_through_weigh_power_on_and_reshow(void)
{
    display_driver_state_t disp;
    weight_driver_state_t weigh;
    static const uint8_t first[TM1637_GRID_COUNT] = {0x00u, 0x66u, 0x5Bu, 0x10u, 0x00u};
    static const uint8_t second[TM1637_GRID_COUNT] = {0x00u, 0x06u, 0x6Du, 0x10u, 0x00u};
    size_t tm1637_after_first;
    size_t tm1637_after_second;

    stack_init(&disp, &weigh);

    TEST_ASSERT_EQUAL(PORT_OK, display_power_on(&disp));
    TEST_ASSERT_EQUAL(PORT_OK, display_show_grids(&disp, first));
    TEST_ASSERT_TRUE(display_rail_is_high());
    (void)fake_tm1637_gpio_bytes(&tm1637_after_first);

    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_enable(&weigh));
    TEST_ASSERT_EQUAL(PORT_OK, weight_boot_settle(&weigh));
    TEST_ASSERT_TRUE(display_rail_is_high());
    TEST_ASSERT_TRUE(weigh_rail_is_high());

    TEST_ASSERT_EQUAL(PORT_OK, display_show_grids(&disp, second));
    (void)fake_tm1637_gpio_bytes(&tm1637_after_second);
    TEST_ASSERT_TRUE(tm1637_after_second > tm1637_after_first);
    TEST_ASSERT_TRUE(display_rail_is_high());
}

void test_weigh_rail_enable_twice_does_not_drop_display_rail(void)
{
    display_driver_state_t disp;
    weight_driver_state_t weigh;

    stack_init(&disp, &weigh);
    TEST_ASSERT_EQUAL(PORT_OK, display_power_on(&disp));
    TEST_ASSERT_TRUE(display_rail_is_high());

    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_enable(&weigh));
    TEST_ASSERT_EQUAL(PORT_OK, weight_boot_settle(&weigh));
    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_enable(&weigh));
    TEST_ASSERT_EQUAL(PORT_OK, weight_boot_settle(&weigh));
    TEST_ASSERT_TRUE(display_rail_is_high());
    TEST_ASSERT_TRUE(weigh_rail_is_high());
}

void test_expander_bootstrap_clears_active_display_and_weigh_rails(void)
{
    display_driver_state_t disp;
    weight_driver_state_t weigh;
    const gpio_expander_port_t *exp;

    stack_init(&disp, &weigh);
    TEST_ASSERT_EQUAL(PORT_OK, display_power_on(&disp));
    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_enable(&weigh));
    TEST_ASSERT_EQUAL(PORT_OK, weight_boot_settle(&weigh));
    TEST_ASSERT_TRUE(display_rail_is_high());
    TEST_ASSERT_TRUE(weigh_rail_is_high());

    exp = fake_gpio_expander_port_get();
    TEST_ASSERT_EQUAL(PORT_OK, gpio_expander_bootstrap(exp));
    TEST_ASSERT_FALSE(display_rail_is_high());
    TEST_ASSERT_FALSE(weigh_rail_is_high());
}
