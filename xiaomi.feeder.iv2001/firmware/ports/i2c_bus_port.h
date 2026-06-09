/*
 * I2C bus port — spec/40-architecture/ports.md
 */

#ifndef I2C_BUS_PORT_H
#define I2C_BUS_PORT_H

#include <stdint.h>

#include "port_err.h"

typedef struct i2c_bus_port {
    port_err_t (*write_reg)(uint8_t addr, uint8_t reg, uint8_t val);
    port_err_t (*read_reg)(uint8_t addr, uint8_t reg, uint8_t *val);
} i2c_bus_port_t;

const i2c_bus_port_t *i2c_bus_port_get(void);

#endif /* I2C_BUS_PORT_H */
