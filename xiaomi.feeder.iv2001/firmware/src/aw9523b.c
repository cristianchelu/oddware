/*
 * AW9523B register shadow — spec/10-hardware/components/gpio-expander-aw9523b.md
 */

#include <stddef.h>

#include "aw9523b.h"

void aw9523b_init(aw9523b_t *dev, const i2c_bus_port_t *i2c, uint8_t addr)
{
    dev->i2c = i2c;
    dev->addr = addr;
    dev->dir_p0 = 0xFFu;
    dev->dir_p1 = 0xFFu;
    dev->out_p0 = 0x00u;
    dev->out_p1 = 0x00u;
    dev->dirty_dir_p0 = false;
    dev->dirty_dir_p1 = false;
    dev->dirty_out_p0 = false;
    dev->dirty_out_p1 = false;
}

port_err_t aw9523b_read_id(const aw9523b_t *dev, uint8_t *id)
{
    if (dev == NULL || dev->i2c == NULL || id == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return dev->i2c->read_reg(dev->addr, AW9523B_REG_ID, id);
}

port_err_t aw9523b_enable_gpio_mode(aw9523b_t *dev)
{
    uint8_t ctl;
    port_err_t err;

    if (dev == NULL || dev->i2c == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = dev->i2c->read_reg(dev->addr, AW9523B_REG_CTL, &ctl);
    if (err != PORT_OK) {
        return err;
    }

    ctl = (uint8_t)(ctl | AW9523B_CTL_GPIO_MODE);
    return dev->i2c->write_reg(dev->addr, AW9523B_REG_CTL, ctl);
}

port_err_t aw9523b_configure(aw9523b_t *dev,
                             uint8_t dir_p0,
                             uint8_t dir_p1,
                             uint8_t out_p0,
                             uint8_t out_p1)
{
    if (dev == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (dev->dir_p0 != dir_p0) {
        dev->dir_p0 = dir_p0;
        dev->dirty_dir_p0 = true;
    }
    if (dev->dir_p1 != dir_p1) {
        dev->dir_p1 = dir_p1;
        dev->dirty_dir_p1 = true;
    }
    if (dev->out_p0 != out_p0) {
        dev->out_p0 = out_p0;
        dev->dirty_out_p0 = true;
    }
    if (dev->out_p1 != out_p1) {
        dev->out_p1 = out_p1;
        dev->dirty_out_p1 = true;
    }

    return PORT_OK;
}

port_err_t aw9523b_set_output_bit(aw9523b_t *dev, uint8_t port, uint8_t pin, bool level)
{
    uint8_t mask;
    uint8_t *out;
    bool *dirty;

    if (dev == NULL || pin > 7u) {
        return PORT_ERR_INVALID_ARG;
    }

    mask = (uint8_t)(1u << pin);
    if (port == 0u) {
        out = &dev->out_p0;
        dirty = &dev->dirty_out_p0;
    } else if (port == 1u) {
        out = &dev->out_p1;
        dirty = &dev->dirty_out_p1;
    } else {
        return PORT_ERR_INVALID_ARG;
    }

    if (level) {
        if ((*out & mask) == mask) {
            return PORT_OK;
        }
        *out = (uint8_t)(*out | mask);
    } else {
        if ((*out & mask) == 0u) {
            return PORT_OK;
        }
        *out = (uint8_t)(*out & (uint8_t)~mask);
    }

    *dirty = true;
    return PORT_OK;
}

port_err_t aw9523b_flush(aw9523b_t *dev)
{
    port_err_t err;

    if (dev == NULL || dev->i2c == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (dev->dirty_dir_p0) {
        err = dev->i2c->write_reg(dev->addr, AW9523B_REG_DIR_P0, dev->dir_p0);
        if (err != PORT_OK) {
            return err;
        }
        dev->dirty_dir_p0 = false;
    }
    if (dev->dirty_dir_p1) {
        err = dev->i2c->write_reg(dev->addr, AW9523B_REG_DIR_P1, dev->dir_p1);
        if (err != PORT_OK) {
            return err;
        }
        dev->dirty_dir_p1 = false;
    }
    if (dev->dirty_out_p0) {
        err = dev->i2c->write_reg(dev->addr, AW9523B_REG_OUTPUT_P0, dev->out_p0);
        if (err != PORT_OK) {
            return err;
        }
        dev->dirty_out_p0 = false;
    }
    if (dev->dirty_out_p1) {
        err = dev->i2c->write_reg(dev->addr, AW9523B_REG_OUTPUT_P1, dev->out_p1);
        if (err != PORT_OK) {
            return err;
        }
        dev->dirty_out_p1 = false;
    }

    return PORT_OK;
}

uint8_t aw9523b_output_get(const aw9523b_t *dev, uint8_t port)
{
    if (dev == NULL) {
        return 0u;
    }

    return (port == 0u) ? dev->out_p0 : dev->out_p1;
}
