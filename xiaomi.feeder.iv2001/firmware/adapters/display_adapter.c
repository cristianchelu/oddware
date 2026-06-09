/*
 * Display port adapter — spec/40-architecture/ports.md
 */

#include "hal_gpio.h"
#include "hal_gpt.h"
#include "hal_pinmux_define.h"

#include "display_driver.h"
#include "display_port.h"
#include "gpio_expander_port.h"
#include "tm1637.h"

#define TM1637_DIO_GPIO  HAL_GPIO_1
#define TM1637_CLK_GPIO  HAL_GPIO_13

static display_driver_state_t s_state;

static void tm1637_hal_set_dio(bool high)
{
    hal_gpio_set_output(TM1637_DIO_GPIO,
                        high ? HAL_GPIO_DATA_HIGH : HAL_GPIO_DATA_LOW);
}

static void tm1637_hal_set_clk(bool high)
{
    hal_gpio_set_output(TM1637_CLK_GPIO,
                        high ? HAL_GPIO_DATA_HIGH : HAL_GPIO_DATA_LOW);
}

static void tm1637_hal_delay_us(uint32_t us)
{
    hal_gpt_delay_us(us);
}

static void display_hal_delay_ms(uint32_t ms)
{
    hal_gpt_delay_ms(ms);
}

static const tm1637_gpio_ops_t s_tm1637_gpio = {
    .set_dio = tm1637_hal_set_dio,
    .set_clk = tm1637_hal_set_clk,
    .delay_us = tm1637_hal_delay_us,
};

static bool s_display_ready;

static void tm1637_prepare_pins(void)
{
    hal_gpio_init(TM1637_DIO_GPIO);
    hal_pinmux_set_function(TM1637_DIO_GPIO, HAL_GPIO_1_GPIO1);
    hal_gpio_set_direction(TM1637_DIO_GPIO, HAL_GPIO_DIRECTION_OUTPUT);
    hal_gpio_pull_up(TM1637_DIO_GPIO);
    tm1637_hal_set_dio(true);

    hal_gpio_init(TM1637_CLK_GPIO);
    hal_pinmux_set_function(TM1637_CLK_GPIO, HAL_GPIO_13_GPIO13);
    hal_gpio_set_direction(TM1637_CLK_GPIO, HAL_GPIO_DIRECTION_OUTPUT);
    hal_gpio_set_pupd_register(TM1637_CLK_GPIO, 0, 1, 0);
    tm1637_hal_set_clk(true);
}

static void display_adapter_ensure_init(void)
{
    if (s_display_ready) {
        return;
    }

    display_hw_t hw = {
        .expander = gpio_expander_port_get(),
        .tm1637_gpio = &s_tm1637_gpio,
        .prepare_tm1637_pins = tm1637_prepare_pins,
        .delay_ms = display_hal_delay_ms,
    };

    display_driver_init(&s_state, &hw);
    s_display_ready = true;
}

static port_err_t port_power_on(void)
{
    display_adapter_ensure_init();
    return display_power_on(&s_state);
}

static port_err_t port_power_off(void)
{
    display_adapter_ensure_init();
    return display_power_off(&s_state);
}

static port_err_t port_show_fill(uint8_t segment_byte)
{
    display_adapter_ensure_init();
    return display_show_fill(&s_state, segment_byte);
}

static port_err_t port_show_grids(const uint8_t grids[TM1637_GRID_COUNT])
{
    display_adapter_ensure_init();
    return display_show_grids(&s_state, grids);
}

static port_err_t port_blank(void)
{
    display_adapter_ensure_init();
    return display_blank(&s_state);
}

static const display_port_t s_display_port = {
    .power_on = port_power_on,
    .power_off = port_power_off,
    .show_fill = port_show_fill,
    .show_grids = port_show_grids,
    .blank = port_blank,
};

const display_port_t *display_port_get(void)
{
    return &s_display_port;
}
