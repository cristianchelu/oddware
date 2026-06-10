/*
 * GPIO expander port adapter — AW9523B via I2C + GPIO14 reset.
 */

#include "hal_gpio.h"
#include "hal_gpt.h"
#include "hal_pinmux_define.h"
#include "syslog.h"

#include "aw9523b.h"
#include "board_gpio_iv2001.h"
#include "gpio_expander_loan.h"
#include "gpio_expander_port.h"
#include "i2c_bus_adapter.h"
#include "i2c_bus_port.h"
#include "wfci_bus_adapter.h"

log_create_module(gpio_exp, PRINT_LEVEL_INFO);

#define AW9523B_RST_GPIO  HAL_GPIO_14
#define AW9523B_RESET_ATTEMPTS  3u

static aw9523b_t s_aw9523b;
static bool s_exp_ready;

static void gpio_expander_adapter_hw_reset(void)
{
    /* Factory / display_hello: level → direction → 100 ms; RST released via EPT input. */
    hal_gpio_init(AW9523B_RST_GPIO);
    hal_pinmux_set_function(AW9523B_RST_GPIO, HAL_GPIO_14_GPIO14);
    hal_gpio_set_output(AW9523B_RST_GPIO, HAL_GPIO_DATA_LOW);
    hal_gpio_set_direction(AW9523B_RST_GPIO, HAL_GPIO_DIRECTION_OUTPUT);
    hal_gpt_delay_ms(100);
}

static port_err_t gpio_exp_reset_once(void)
{
    uint8_t id;
    port_err_t err;

    gpio_expander_adapter_hw_reset();
    if (!wfci_bus_wifi_spi_active_get()) {
        i2c_bus_adapter_init();
    }

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
    bool nested = gpio_expander_loan_is_held();

    if (!nested) {
        err = gpio_expander_loan_begin();
        if (err != PORT_OK) {
            return err;
        }
    }

    for (attempt = 0u; attempt < AW9523B_RESET_ATTEMPTS; attempt++) {
        err = gpio_exp_reset_once();
        if (err == PORT_OK) {
            if (!nested) {
                gpio_expander_loan_end();
            }
            return PORT_OK;
        }
        if (!wfci_bus_wifi_spi_active_get()) {
            i2c_bus_adapter_deinit();
        }
        hal_gpt_delay_ms(50);
    }

    if (!nested) {
        gpio_expander_loan_end();
    }
    return PORT_ERR_IO;
}

static port_err_t gpio_exp_with_loan(port_err_t (*fn)(void))
{
    bool nested = gpio_expander_loan_is_held();
    port_err_t err;

    if (!nested) {
        err = gpio_expander_loan_begin();
        if (err != PORT_OK) {
            return err;
        }
    }

    err = fn();

    if (!nested) {
        gpio_expander_loan_end();
    }
    return err;
}

static uint8_t s_cfg_dir_p0;
static uint8_t s_cfg_dir_p1;
static uint8_t s_cfg_out_p0;
static uint8_t s_cfg_out_p1;
static uint8_t s_pin_port;
static uint8_t s_pin_pin;
static bool s_pin_level;

static port_err_t gpio_exp_configure_body(void)
{
    port_err_t err;

    err = aw9523b_configure(&s_aw9523b,
                              s_cfg_dir_p0,
                              s_cfg_dir_p1,
                              s_cfg_out_p0,
                              s_cfg_out_p1);
    if (err != PORT_OK) {
        return err;
    }
    return aw9523b_flush(&s_aw9523b);
}

static port_err_t gpio_exp_configure(uint8_t dir_p0,
                                     uint8_t dir_p1,
                                     uint8_t out_p0,
                                     uint8_t out_p1)
{
    s_cfg_dir_p0 = dir_p0;
    s_cfg_dir_p1 = dir_p1;
    s_cfg_out_p0 = out_p0;
    s_cfg_out_p1 = out_p1;
    return gpio_exp_with_loan(gpio_exp_configure_body);
}

static port_err_t gpio_exp_set_pin_body(void)
{
    port_err_t err;

    err = aw9523b_set_output_bit(&s_aw9523b, s_pin_port, s_pin_pin, s_pin_level);
    if (err != PORT_OK) {
        return err;
    }
    return aw9523b_flush(&s_aw9523b);
}

static port_err_t gpio_exp_set_pin(uint8_t port, uint8_t pin, bool level)
{
    s_pin_port = port;
    s_pin_pin = pin;
    s_pin_level = level;
    return gpio_exp_with_loan(gpio_exp_set_pin_body);
}

static uint8_t s_int_mask_p0;
static uint8_t s_int_mask_p1;

static port_err_t gpio_exp_read_inputs(uint8_t *p0, uint8_t *p1)
{
    port_err_t err;
    bool nested = gpio_expander_loan_is_held();

    if (p0 == NULL || p1 == NULL || !s_exp_ready) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!nested) {
        err = gpio_expander_loan_begin();
        if (err != PORT_OK) {
            return err;
        }
    }

    err = aw9523b_read_inputs(&s_aw9523b, p0, p1);

    if (!nested) {
        gpio_expander_loan_end();
    }
    return err;
}

static port_err_t gpio_exp_set_int_mask_body(void)
{
    return aw9523b_set_int_mask(&s_aw9523b, s_int_mask_p0, s_int_mask_p1);
}

static port_err_t gpio_exp_set_int_mask(uint8_t mask_p0, uint8_t mask_p1)
{
    s_int_mask_p0 = mask_p0;
    s_int_mask_p1 = mask_p1;
    return gpio_exp_with_loan(gpio_exp_set_int_mask_body);
}

static port_err_t gpio_exp_get_pin(uint8_t port, uint8_t pin, bool *level)
{
    uint8_t reg;
    uint8_t val;
    port_err_t err;
    bool nested = gpio_expander_loan_is_held();

    if (level == NULL || !s_exp_ready) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!nested) {
        err = gpio_expander_loan_begin();
        if (err != PORT_OK) {
            return err;
        }
    }

    reg = (port == 0u) ? AW9523B_REG_INPUT_P0 : AW9523B_REG_INPUT_P1;
    err = s_aw9523b.i2c->read_reg(s_aw9523b.addr, reg, &val);
    if (err == PORT_OK) {
        *level = (val & (uint8_t)(1u << pin)) != 0u;
    }

    if (!nested) {
        gpio_expander_loan_end();
    }
    return err;
}

static const gpio_expander_port_t s_gpio_expander = {
    .reset = gpio_exp_reset,
    .configure = gpio_exp_configure,
    .set_pin = gpio_exp_set_pin,
    .get_pin = gpio_exp_get_pin,
    .read_inputs = gpio_exp_read_inputs,
    .set_int_mask = gpio_exp_set_int_mask,
};

const gpio_expander_port_t *gpio_expander_port_get(void)
{
    return &s_gpio_expander;
}
