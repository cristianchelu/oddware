/* Tests: spec/10-hardware/components/display-tm1637.md */

#include <stdbool.h>

#include "unity.h"

#include "fake_tm1637_gpio.h"
#include "tm1637.h"

static bool bytes_contain_subsequence(const uint8_t *buf,
                                      size_t count,
                                      const uint8_t *seq,
                                      size_t seq_len)
{
    if (seq_len == 0u || count < seq_len) {
        return false;
    }

    for (size_t i = 0; i + seq_len <= count; i++) {
        bool match = true;
        for (size_t j = 0; j < seq_len; j++) {
            if (buf[i + j] != seq[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

void test_tm1637_refresh_per_grid_sequence(void)
{
    uint8_t grids[TM1637_GRID_COUNT] = {0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
    const uint8_t *bytes;
    size_t count;
    const uint8_t grid0_prefix[] = {TM1637_CMD_FIXED_ADDR, 0xC0u, 0xFFu};
    const uint8_t grid5_clear[] = {TM1637_CMD_FIXED_ADDR, 0xC5u, 0x00u};

    fake_tm1637_gpio_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      tm1637_refresh(fake_tm1637_gpio_ops_get(), grids, TM1637_BRIGHTNESS_MAX));

    bytes = fake_tm1637_gpio_bytes(&count);
    TEST_ASSERT_TRUE(count > 0u);
    TEST_ASSERT_TRUE(bytes_contain_subsequence(bytes, count, grid0_prefix, sizeof(grid0_prefix)));
    TEST_ASSERT_TRUE(bytes_contain_subsequence(bytes, count, grid5_clear, sizeof(grid5_clear)));
    {
        uint8_t brightness = TM1637_BRIGHTNESS_MAX;
        TEST_ASSERT_TRUE(bytes_contain_subsequence(bytes, count, &brightness, 1u));
    }
}

void test_tm1637_refresh_all_zeros(void)
{
    uint8_t grids[TM1637_GRID_COUNT] = {0};
    const uint8_t *bytes;
    size_t count;
    const uint8_t grid2_prefix[] = {TM1637_CMD_FIXED_ADDR, 0xC2u, 0x00u};

    fake_tm1637_gpio_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      tm1637_refresh(fake_tm1637_gpio_ops_get(), grids, TM1637_BRIGHTNESS_MAX));

    bytes = fake_tm1637_gpio_bytes(&count);
    TEST_ASSERT_TRUE(bytes_contain_subsequence(bytes, count, grid2_prefix, sizeof(grid2_prefix)));
}
