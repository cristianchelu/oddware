#include "fake_i2c_bus.h"

#define FAKE_I2C_MAX_OPS 32u

static fake_i2c_op_t s_ops[FAKE_I2C_MAX_OPS];
static size_t s_op_count;
static port_err_t s_read_err = PORT_OK;
static uint8_t s_read_val;

void fake_i2c_bus_reset(void)
{
    s_op_count = 0;
    s_read_err = PORT_OK;
    s_read_val = 0u;
}

void fake_i2c_bus_set_read_result(port_err_t err, uint8_t val)
{
    s_read_err = err;
    s_read_val = val;
}

const fake_i2c_op_t *fake_i2c_bus_ops(size_t *count)
{
    if (count != NULL) {
        *count = s_op_count;
    }
    return s_ops;
}

static void fake_i2c_record(bool is_write, uint8_t addr, uint8_t reg, uint8_t val)
{
    if (s_op_count >= FAKE_I2C_MAX_OPS) {
        return;
    }

    s_ops[s_op_count].is_write = is_write;
    s_ops[s_op_count].addr = addr;
    s_ops[s_op_count].reg = reg;
    s_ops[s_op_count].val = val;
    s_op_count++;
}

static port_err_t fake_i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    fake_i2c_record(true, addr, reg, val);
    return PORT_OK;
}

static port_err_t fake_i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *val)
{
    fake_i2c_record(false, addr, reg, 0u);
    if (val == NULL) {
        return PORT_ERR_INVALID_ARG;
    }
    if (s_read_err != PORT_OK) {
        return s_read_err;
    }
    *val = s_read_val;
    return PORT_OK;
}

static const i2c_bus_port_t s_fake_i2c_bus = {
    .write_reg = fake_i2c_write_reg,
    .read_reg = fake_i2c_read_reg,
};

const i2c_bus_port_t *fake_i2c_bus_port_get(void)
{
    return &s_fake_i2c_bus;
}
