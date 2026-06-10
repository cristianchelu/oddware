/*
 * Display port adapter — spec/40-architecture/ports.md
 */

#include "FreeRTOS.h"
#include "semphr.h"

#include "hal_gpio.h"
#include "hal_gpt.h"
#include "hal_pinmux_define.h"

#include "display_driver.h"
#include "display_port.h"
#include "display_presentation.h"
#include "display_rail.h"
#include "gpio_expander_port.h"
#include "tm1637.h"
#include "wfci_bus_port.h"

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

void gpio_expander_outputs_reset_hook(void)
{
    s_state.powered = false;
    display_rail_invalidate(&s_state.rail);
    display_presentation_note_expander_reset();
}

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

typedef port_err_t (*display_op_fn_t)(void);

static SemaphoreHandle_t s_display_bus_mutex;

static void display_bus_mutex_ensure(void)
{
    if (s_display_bus_mutex == NULL) {
        s_display_bus_mutex = xSemaphoreCreateMutex();
    }
}

static port_err_t display_with_bus(display_op_fn_t fn)
{
    const wfci_bus_port_t *bus = wfci_bus_port_get();
    port_err_t err;

    display_bus_mutex_ensure();
    if (xSemaphoreTake(s_display_bus_mutex, pdMS_TO_TICKS(5000)) != pdPASS) {
        return PORT_ERR_BUSY;
    }

    err = bus->acquire(WFCI_BUS_PROFILE_DISPLAY, WFCI_BUS_PRIORITY_NORMAL, 5000u);
    if (err != PORT_OK) {
        (void)xSemaphoreGive(s_display_bus_mutex);
        return err;
    }

    display_adapter_ensure_init();
    err = fn();
    bus->release(WFCI_BUS_PROFILE_DISPLAY);
    (void)xSemaphoreGive(s_display_bus_mutex);
    return err;
}

static port_err_t port_power_on_body(void)
{
    return display_power_on(&s_state);
}

static port_err_t port_power_on(void)
{
    return display_with_bus(port_power_on_body);
}

static port_err_t port_power_off_body(void)
{
    return display_power_off(&s_state);
}

static port_err_t port_power_off(void)
{
    return display_with_bus(port_power_off_body);
}

static uint8_t s_fill_byte;

static port_err_t port_show_fill_body(void)
{
    return display_show_fill(&s_state, s_fill_byte);
}

static port_err_t port_show_fill(uint8_t segment_byte)
{
    s_fill_byte = segment_byte;
    return display_with_bus(port_show_fill_body);
}

static const uint8_t *s_grids_ptr;

static port_err_t port_show_grids_body(void)
{
    return display_show_grids(&s_state, s_grids_ptr);
}

static port_err_t port_show_grids(const uint8_t grids[TM1637_GRID_COUNT])
{
    s_grids_ptr = grids;
    return display_with_bus(port_show_grids_body);
}

static port_err_t port_blank_body(void)
{
    return display_blank(&s_state);
}

static port_err_t port_blank(void)
{
    return display_with_bus(port_blank_body);
}

static uint8_t s_brightness_level;

static port_err_t port_set_brightness_body(void)
{
    return display_set_brightness(&s_state, s_brightness_level);
}

static port_err_t port_set_brightness(uint8_t level)
{
    s_brightness_level = level;
    display_adapter_ensure_init();
    return port_set_brightness_body();
}

static const display_port_t s_display_port = {
    .power_on = port_power_on,
    .power_off = port_power_off,
    .show_fill = port_show_fill,
    .show_grids = port_show_grids,
    .blank = port_blank,
    .set_brightness = port_set_brightness,
};

const display_port_t *display_port_get(void)
{
    return &s_display_port;
}
