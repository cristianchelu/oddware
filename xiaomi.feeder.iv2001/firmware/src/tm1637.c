/*
 * TM1637 bit-bang — spec/10-hardware/components/display-tm1637.md
 */

#include <stddef.h>

#include "tm1637.h"

#define TM1637_HALF_PERIOD_US  10u

static void tm1637_delay(const tm1637_gpio_ops_t *gpio)
{
    if (gpio != NULL && gpio->delay_us != NULL) {
        gpio->delay_us(TM1637_HALF_PERIOD_US);
    }
}

static void tm1637_start(const tm1637_gpio_ops_t *gpio)
{
    gpio->set_dio(true);
    gpio->set_clk(true);
    tm1637_delay(gpio);
    gpio->set_dio(false);
    tm1637_delay(gpio);
    gpio->set_clk(false);
    tm1637_delay(gpio);
}

static void tm1637_stop(const tm1637_gpio_ops_t *gpio)
{
    gpio->set_dio(false);
    gpio->set_clk(true);
    tm1637_delay(gpio);
    gpio->set_dio(true);
    tm1637_delay(gpio);
    gpio->set_clk(false);
    tm1637_delay(gpio);
}

static void tm1637_write_byte(const tm1637_gpio_ops_t *gpio, uint8_t value)
{
    uint8_t orig = value;

    for (uint8_t i = 0u; i < 8u; i++) {
        gpio->set_clk(false);
        tm1637_delay(gpio);
        gpio->set_dio((value & 0x01u) != 0u);
        tm1637_delay(gpio);
        gpio->set_clk(true);
        tm1637_delay(gpio);
        value = (uint8_t)(value >> 1);
    }

    gpio->set_clk(false);
    tm1637_delay(gpio);

    if (gpio->on_byte != NULL) {
        gpio->on_byte(orig);
    }
}

static void tm1637_command(const tm1637_gpio_ops_t *gpio, uint8_t cmd)
{
    tm1637_start(gpio);
    tm1637_write_byte(gpio, cmd);
    tm1637_stop(gpio);
}

static void tm1637_write_grid(const tm1637_gpio_ops_t *gpio, uint8_t grid, uint8_t seg)
{
    tm1637_command(gpio, TM1637_CMD_FIXED_ADDR);
    tm1637_start(gpio);
    tm1637_write_byte(gpio, (uint8_t)(0xC0u | grid));
    tm1637_write_byte(gpio, seg);
    tm1637_stop(gpio);
}

port_err_t tm1637_refresh(const tm1637_gpio_ops_t *gpio,
                          const uint8_t grids[TM1637_GRID_COUNT],
                          uint8_t brightness_cmd)
{
    if (gpio == NULL || gpio->set_dio == NULL || gpio->set_clk == NULL || grids == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    for (uint8_t grid = 0u; grid < TM1637_GRID_COUNT; grid++) {
        tm1637_write_grid(gpio, grid, grids[grid]);
    }

    tm1637_write_grid(gpio, 5u, 0x00u);
    tm1637_command(gpio, brightness_cmd);

    return PORT_OK;
}
