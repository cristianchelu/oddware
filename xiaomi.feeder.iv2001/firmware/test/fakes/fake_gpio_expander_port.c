#include "fake_gpio_expander_port.h"

#include "aw9523b.h"
#include "board_gpio_iv2001.h"
#include "fake_i2c_bus.h"

static aw9523b_t s_dev;
static port_err_t s_reset_err = PORT_OK;
static bool s_dev_ready;
static bool s_id_pinned;
static uint8_t s_input_p0 = 0xFFu;
static uint8_t s_input_p1 = 0xFFu;

static port_err_t fake_exp_reset(void)
{
    uint8_t id;
    port_err_t err;

    if (s_reset_err != PORT_OK) {
        return s_reset_err;
    }

    if (!s_dev_ready) {
        aw9523b_init(&s_dev, fake_i2c_bus_port_get(), BOARD_GPIO_AW9523B_ADDR);
        s_dev_ready = true;
    }

    if (!s_id_pinned) {
        fake_i2c_bus_set_read_result(PORT_OK, AW9523B_ID_EXPECTED);
    }
    err = aw9523b_read_id(&s_dev, &id);
    if (err != PORT_OK) {
        return err;
    }
    if (id != AW9523B_ID_EXPECTED) {
        return PORT_ERR_IO;
    }

    fake_i2c_bus_set_read_result(PORT_OK, 0x00u);
    return aw9523b_enable_gpio_mode(&s_dev);
}

static port_err_t fake_exp_configure(uint8_t dir_p0,
                                     uint8_t dir_p1,
                                     uint8_t out_p0,
                                     uint8_t out_p1)
{
    port_err_t err;

    err = aw9523b_configure(&s_dev, dir_p0, dir_p1, out_p0, out_p1);
    if (err != PORT_OK) {
        return err;
    }
    return aw9523b_flush(&s_dev);
}

static port_err_t fake_exp_set_pin(uint8_t port, uint8_t pin, bool level)
{
    port_err_t err;

    err = aw9523b_set_output_bit(&s_dev, port, pin, level);
    if (err != PORT_OK) {
        return err;
    }
    return aw9523b_flush(&s_dev);
}

static port_err_t fake_exp_get_pin(uint8_t port, uint8_t pin, bool *level)
{
    uint8_t sample;

    if (level == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    sample = (port == 0u) ? s_input_p0 : s_input_p1;
    *level = (sample & (uint8_t)(1u << pin)) != 0u;
    return PORT_OK;
}

static port_err_t fake_exp_read_inputs(uint8_t *p0, uint8_t *p1)
{
    if (p0 == NULL || p1 == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    *p0 = s_input_p0;
    *p1 = s_input_p1;
    return PORT_OK;
}

static port_err_t fake_exp_set_int_mask(uint8_t mask_p0, uint8_t mask_p1)
{
    (void)mask_p0;
    (void)mask_p1;
    return PORT_OK;
}

static const gpio_expander_port_t s_fake_exp = {
    .reset = fake_exp_reset,
    .configure = fake_exp_configure,
    .set_pin = fake_exp_set_pin,
    .get_pin = fake_exp_get_pin,
    .read_inputs = fake_exp_read_inputs,
    .set_int_mask = fake_exp_set_int_mask,
};

void fake_gpio_expander_reset(void)
{
    s_reset_err = PORT_OK;
    s_dev_ready = false;
    s_id_pinned = false;
    s_input_p0 = 0xFFu;
    s_input_p1 = 0xFFu;
    fake_i2c_bus_reset();
    fake_i2c_bus_set_read_result(PORT_OK, AW9523B_ID_EXPECTED);
}

void fake_gpio_expander_set_inputs(uint8_t p0, uint8_t p1)
{
    s_input_p0 = p0;
    s_input_p1 = p1;
}

void fake_gpio_expander_set_reset_err(port_err_t err)
{
    s_reset_err = err;
}

void fake_gpio_expander_set_id(uint8_t id)
{
    s_id_pinned = true;
    fake_i2c_bus_set_read_result(PORT_OK, id);
}

uint8_t fake_gpio_expander_out_p0(void)
{
    return aw9523b_output_get(&s_dev, 0u);
}

bool fake_gpio_expander_pin(uint8_t port, uint8_t pin)
{
    uint8_t out = aw9523b_output_get(&s_dev, port);
    return (out & (uint8_t)(1u << pin)) != 0u;
}

const gpio_expander_port_t *fake_gpio_expander_port_get(void)
{
    return &s_fake_exp;
}

const gpio_expander_port_t *gpio_expander_port_get(void)
{
    return fake_gpio_expander_port_get();
}
