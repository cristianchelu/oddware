#include <stdbool.h>

#include "fake_tm1637_gpio.h"

#define FAKE_TM1637_MAX_BYTES 64u

static uint8_t s_bytes[FAKE_TM1637_MAX_BYTES];
static size_t s_byte_count;

static void fake_tm1637_record_byte(uint8_t b)
{
    if (s_byte_count < FAKE_TM1637_MAX_BYTES) {
        s_bytes[s_byte_count++] = b;
    }
}

static void fake_tm1637_set_dio(bool high)
{
    (void)high;
}

static void fake_tm1637_set_clk(bool high)
{
    (void)high;
}

static void fake_tm1637_delay_us(uint32_t us)
{
    (void)us;
}

void fake_tm1637_gpio_reset(void)
{
    s_byte_count = 0u;
}

const uint8_t *fake_tm1637_gpio_bytes(size_t *count)
{
    if (count != NULL) {
        *count = s_byte_count;
    }
    return s_bytes;
}

static const tm1637_gpio_ops_t s_ops = {
    .set_dio = fake_tm1637_set_dio,
    .set_clk = fake_tm1637_set_clk,
    .delay_us = fake_tm1637_delay_us,
    .on_byte = fake_tm1637_record_byte,
};

const tm1637_gpio_ops_t *fake_tm1637_gpio_ops_get(void)
{
    return &s_ops;
}
