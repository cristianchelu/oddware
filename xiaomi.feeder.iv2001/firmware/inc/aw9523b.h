/*
 * AW9523B register model — spec/10-hardware/components/gpio-expander-aw9523b.md
 */

#ifndef AW9523B_H
#define AW9523B_H

#include <stdbool.h>
#include <stdint.h>

#include "i2c_bus_port.h"
#include "port_err.h"

#define AW9523B_REG_INPUT_P0   0x00u
#define AW9523B_REG_INPUT_P1   0x01u
#define AW9523B_REG_OUTPUT_P0  0x02u
#define AW9523B_REG_OUTPUT_P1  0x03u
#define AW9523B_REG_DIR_P0     0x04u
#define AW9523B_REG_DIR_P1     0x05u
#define AW9523B_REG_ID         0x10u
#define AW9523B_REG_CTL        0x11u
#define AW9523B_ID_EXPECTED    0x23u
#define AW9523B_CTL_GPIO_MODE  0x10u

typedef struct aw9523b {
    const i2c_bus_port_t *i2c;
    uint8_t addr;
    uint8_t dir_p0;
    uint8_t dir_p1;
    uint8_t out_p0;
    uint8_t out_p1;
    bool dirty_dir_p0;
    bool dirty_dir_p1;
    bool dirty_out_p0;
    bool dirty_out_p1;
} aw9523b_t;

void aw9523b_init(aw9523b_t *dev, const i2c_bus_port_t *i2c, uint8_t addr);

port_err_t aw9523b_read_id(const aw9523b_t *dev, uint8_t *id);

port_err_t aw9523b_enable_gpio_mode(aw9523b_t *dev);

port_err_t aw9523b_configure(aw9523b_t *dev,
                             uint8_t dir_p0,
                             uint8_t dir_p1,
                             uint8_t out_p0,
                             uint8_t out_p1);

port_err_t aw9523b_set_output_bit(aw9523b_t *dev, uint8_t port, uint8_t pin, bool level);

port_err_t aw9523b_flush(aw9523b_t *dev);

uint8_t aw9523b_output_get(const aw9523b_t *dev, uint8_t port);

#endif /* AW9523B_H */
