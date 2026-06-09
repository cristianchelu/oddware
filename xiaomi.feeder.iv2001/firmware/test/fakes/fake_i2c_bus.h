#ifndef FAKE_I2C_BUS_H
#define FAKE_I2C_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "i2c_bus_port.h"
#include "port_err.h"

typedef struct fake_i2c_op {
    bool is_write;
    uint8_t addr;
    uint8_t reg;
    uint8_t val;
} fake_i2c_op_t;

void fake_i2c_bus_reset(void);
void fake_i2c_bus_set_read_result(port_err_t err, uint8_t val);
const fake_i2c_op_t *fake_i2c_bus_ops(size_t *count);
const i2c_bus_port_t *fake_i2c_bus_port_get(void);

#endif /* FAKE_I2C_BUS_H */
