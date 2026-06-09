/*
 * GPIO expander port adapter — AW9523B via I2C + GPIO14 reset.
 */

#include "hal_gpio.h"
#include "hal_gpt.h"
#include "hal_pinmux_define.h"
#include "syslog.h"

#include "aw9523b.h"
#include "board_gpio_iv2001.h"
#include "gpio_expander_port.h"
#include "i2c_bus_adapter.h"
#include "i2c_bus_port.h"

log_create_module(gpio_exp, PRINT_LEVEL_INFO);

#define AW9523B_RST_GPIO  HAL_GPIO_14
#define AW9523B_RESET_ATTEMPTS  3u

static aw9523b_t s_aw9523b;
static bool s_exp_ready;

static void gpio_expander_adapter_hw_reset(void)
{
    /* 100 ms active-low pulse on GPIO14 — bench-verified on IV2001 `[probe]`. */
    hal_gpio_init(AW9523B_RST_GPIO);
    hal_pinmux_set_function(AW9523B_RST_GPIO, HAL_GPIO_14_GPIO14);
    hal_gpio_set_direction(AW9523B_RST_GPIO, HAL_GPIO_DIRECTION_OUTPUT);
    hal_gpio_set_output(AW9523B_RST_GPIO, HAL_GPIO_DATA_LOW);
    hal_gpt_delay_ms(100);
    hal_gpio_set_output(AW9523B_RST_GPIO, HAL_GPIO_DATA_HIGH);
}

static port_err_t gpio_exp_reset_once(void)
{
    uint8_t id;
    port_err_t err;

    gpio_expander_adapter_hw_reset();
    i2c_bus_adapter_deinit();
    i2c_bus_adapter_init();

    if (!s_exp_ready) {
        aw9523b_init(&s_aw9523b, i2c_bus_port_get(), BOARD_GPIO_AW9523B_ADDR);
        s_exp_ready = true;
    }

    err = aw9523b_read_id(&s_aw9523b, &id);
    if (err != PORT_OK) {
        LOG_E(gpio_exp, "AW9523B ID read failed");
        return err;
    }
    if (id != AW9523B_ID_EXPECTED) {
        LOG_E(gpio_exp, "AW9523B ID=0x%02x expected 0x%02x", id, AW9523B_ID_EXPECTED);
        return PORT_ERR_IO;
    }

    LOG_I(gpio_exp, "AW9523B ID=0x%02x OK", id);

    return aw9523b_enable_gpio_mode(&s_aw9523b);
}

static port_err_t gpio_exp_reset(void)
{
    port_err_t err;
    uint8_t attempt;

    for (attempt = 0u; attempt < AW9523B_RESET_ATTEMPTS; attempt++) {
        err = gpio_exp_reset_once();
        if (err == PORT_OK) {
            return PORT_OK;
        }
        i2c_bus_adapter_deinit();
        hal_gpt_delay_ms(50);
    }

    return PORT_ERR_IO;
}

static port_err_t gpio_exp_configure(uint8_t dir_p0,
                                     uint8_t dir_p1,
                                     uint8_t out_p0,
                                     uint8_t out_p1)
{
    port_err_t err;

    err = aw9523b_configure(&s_aw9523b, dir_p0, dir_p1, out_p0, out_p1);
    if (err != PORT_OK) {
        return err;
    }
    return aw9523b_flush(&s_aw9523b);
}

static port_err_t gpio_exp_set_pin(uint8_t port, uint8_t pin, bool level)
{
    port_err_t err;

    err = aw9523b_set_output_bit(&s_aw9523b, port, pin, level);
    if (err != PORT_OK) {
        return err;
    }
    return aw9523b_flush(&s_aw9523b);
}

static port_err_t gpio_exp_get_pin(uint8_t port, uint8_t pin, bool *level)
{
    uint8_t reg;
    uint8_t val;
    port_err_t err;

    if (level == NULL || !s_exp_ready) {
        return PORT_ERR_INVALID_ARG;
    }

    reg = (port == 0u) ? AW9523B_REG_INPUT_P0 : AW9523B_REG_INPUT_P1;
    err = s_aw9523b.i2c->read_reg(s_aw9523b.addr, reg, &val);
    if (err != PORT_OK) {
        return err;
    }

    *level = (val & (uint8_t)(1u << pin)) != 0u;
    return PORT_OK;
}

static const gpio_expander_port_t s_gpio_expander = {
    .reset = gpio_exp_reset,
    .configure = gpio_exp_configure,
    .set_pin = gpio_exp_set_pin,
    .get_pin = gpio_exp_get_pin,
};

const gpio_expander_port_t *gpio_expander_port_get(void)
{
    return &s_gpio_expander;
}
