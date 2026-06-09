#ifndef FAKE_TM1637_GPIO_H
#define FAKE_TM1637_GPIO_H

#include <stddef.h>
#include <stdint.h>

#include "tm1637.h"

void fake_tm1637_gpio_reset(void);
const uint8_t *fake_tm1637_gpio_bytes(size_t *count);
const tm1637_gpio_ops_t *fake_tm1637_gpio_ops_get(void);

#endif /* FAKE_TM1637_GPIO_H */
