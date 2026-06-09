/*
 * I2C bus port adapter — HAL I2C1 on GPIO15 (SCL) / GPIO16 (SDA) for AW9523B.
 *
 * Boot-time only: WFCI SPI (connsys_init) reclaims GPIO12–16. See
 * spec/40-architecture/build-integration.md § Display boot before Wi-Fi SPI.
 */

#include "hal_gpio.h"
#include "hal_i2c_master.h"
#include "hal_pinmux_define.h"
#include "syslog.h"

#include "board_gpio_iv2001.h"
#include "i2c_bus_adapter.h"
#include "i2c_bus_port.h"
#include "port_err.h"

log_create_module(i2c_bus, PRINT_LEVEL_INFO);

#define I2C_PORT  HAL_I2C_MASTER_1
#define I2C_SCL   ((hal_gpio_pin_t)BOARD_GPIO_I2C_SCL_PIN)
#define I2C_SDA   ((hal_gpio_pin_t)BOARD_GPIO_I2C_SDA_PIN)

static bool s_i2c_ready;

static void i2c_gpio_set_input_pullup(hal_gpio_pin_t pin)
{
    hal_gpio_init(pin);
    hal_gpio_set_pupd_register(pin, 0, 1, 0);
}

static void i2c_pin_init(void)
{
    i2c_gpio_set_input_pullup(I2C_SDA);
    hal_pinmux_set_function(I2C_SDA, HAL_GPIO_16_SDA1);
    i2c_gpio_set_input_pullup(I2C_SCL);
    hal_pinmux_set_function(I2C_SCL, HAL_GPIO_15_SCL1);
}

void i2c_bus_adapter_deinit(void)
{
    if (!s_i2c_ready) {
        return;
    }

    hal_i2c_master_deinit(I2C_PORT);
    s_i2c_ready = false;
}

void i2c_bus_adapter_init(void)
{
    hal_i2c_config_t cfg;

    if (s_i2c_ready) {
        return;
    }

    hal_i2c_master_deinit(I2C_PORT);
    i2c_pin_init();

    cfg.frequency = HAL_I2C_FREQUENCY_400K;
    if (hal_i2c_master_init(I2C_PORT, &cfg) != HAL_I2C_STATUS_OK) {
        LOG_E(i2c_bus, "I2C1 init failed");
        return;
    }

    s_i2c_ready = true;
}

static port_err_t i2c_map_status(hal_i2c_status_t status)
{
    return (status == HAL_I2C_STATUS_OK) ? PORT_OK : PORT_ERR_IO;
}

static port_err_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2];

    if (!s_i2c_ready) {
        return PORT_ERR_IO;
    }

    buf[0] = reg;
    buf[1] = val;
    return i2c_map_status(hal_i2c_master_send_polling(I2C_PORT, addr, buf, 2));
}

static port_err_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *val)
{
    hal_i2c_send_to_receive_config_t cfg;

    if (!s_i2c_ready || val == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    cfg.slave_address = addr;
    cfg.send_data = &reg;
    cfg.send_length = 1;
    cfg.receive_buffer = val;
    cfg.receive_length = 1;
    return i2c_map_status(hal_i2c_master_send_to_receive_polling(I2C_PORT, &cfg));
}

static const i2c_bus_port_t s_i2c_bus = {
    .write_reg = i2c_write_reg,
    .read_reg = i2c_read_reg,
};

const i2c_bus_port_t *i2c_bus_port_get(void)
{
    return &s_i2c_bus;
}
