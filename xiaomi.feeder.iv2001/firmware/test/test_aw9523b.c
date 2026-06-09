/* Tests: spec/30-processes/display-driver.md, spec/10-hardware/components/gpio-expander-aw9523b.md */

#include "unity.h"

#include "aw9523b.h"
#include "board_gpio_iv2001.h"
#include "fake_i2c_bus.h"

void test_aw9523b_configure_flushes_direction_then_output(void)
{
    aw9523b_t dev;
    const fake_i2c_op_t *ops;
    size_t count;

    fake_i2c_bus_reset();
    aw9523b_init(&dev, fake_i2c_bus_port_get(), BOARD_GPIO_AW9523B_ADDR);

    TEST_ASSERT_EQUAL(PORT_OK,
                      aw9523b_configure(&dev,
                                        BOARD_GPIO_BOOT_DIR_P0,
                                        BOARD_GPIO_BOOT_DIR_P1,
                                        BOARD_GPIO_BOOT_OUT_P0,
                                        BOARD_GPIO_BOOT_OUT_P1));
    TEST_ASSERT_EQUAL(PORT_OK, aw9523b_flush(&dev));

    ops = fake_i2c_bus_ops(&count);
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_TRUE(ops[0].is_write);
    TEST_ASSERT_EQUAL(AW9523B_REG_DIR_P0, ops[0].reg);
    TEST_ASSERT_EQUAL(BOARD_GPIO_BOOT_DIR_P0, ops[0].val);
    TEST_ASSERT_EQUAL(AW9523B_REG_DIR_P1, ops[1].reg);
    /* BOOT_OUT all zero — shadow matches reset, no OUTPUT flush */
}

void test_aw9523b_set_output_bit_rmw_preserves_other_bits(void)
{
    aw9523b_t dev;
    const fake_i2c_op_t *ops;
    size_t count;

    fake_i2c_bus_reset();
    aw9523b_init(&dev, fake_i2c_bus_port_get(), BOARD_GPIO_AW9523B_ADDR);
    aw9523b_configure(&dev, 0x00u, 0x00u, BOARD_GPIO_MOTOR_EN_MASK, 0x00u);
    aw9523b_flush(&dev);
    fake_i2c_bus_reset();

    TEST_ASSERT_EQUAL(PORT_OK, aw9523b_set_output_bit(&dev, 0u, 5u, true));
    TEST_ASSERT_EQUAL(PORT_OK, aw9523b_flush(&dev));

    TEST_ASSERT_EQUAL((uint8_t)(BOARD_GPIO_MOTOR_EN_MASK | BOARD_GPIO_DISPLAY_RAIL_MASK),
                      aw9523b_output_get(&dev, 0u));

    ops = fake_i2c_bus_ops(&count);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL((uint8_t)(BOARD_GPIO_MOTOR_EN_MASK | BOARD_GPIO_DISPLAY_RAIL_MASK),
                      ops[0].val);
}

void test_aw9523b_read_id_failure(void)
{
    aw9523b_t dev;
    uint8_t id = 0u;

    fake_i2c_bus_reset();
    fake_i2c_bus_set_read_result(PORT_ERR_IO, 0u);
    aw9523b_init(&dev, fake_i2c_bus_port_get(), BOARD_GPIO_AW9523B_ADDR);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, aw9523b_read_id(&dev, &id));
}

void test_aw9523b_read_id_ok(void)
{
    aw9523b_t dev;
    uint8_t id = 0u;

    fake_i2c_bus_reset();
    fake_i2c_bus_set_read_result(PORT_OK, AW9523B_ID_EXPECTED);
    aw9523b_init(&dev, fake_i2c_bus_port_get(), BOARD_GPIO_AW9523B_ADDR);

    TEST_ASSERT_EQUAL(PORT_OK, aw9523b_read_id(&dev, &id));
    TEST_ASSERT_EQUAL(AW9523B_ID_EXPECTED, id);
}

void test_aw9523b_enable_gpio_mode_sets_ctl_bit(void)
{
    aw9523b_t dev;
    const fake_i2c_op_t *ops;
    size_t count;

    fake_i2c_bus_reset();
    aw9523b_init(&dev, fake_i2c_bus_port_get(), BOARD_GPIO_AW9523B_ADDR);

    fake_i2c_bus_set_read_result(PORT_OK, 0x05u);
    TEST_ASSERT_EQUAL(PORT_OK, aw9523b_enable_gpio_mode(&dev));

    ops = fake_i2c_bus_ops(&count);
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_FALSE(ops[0].is_write);
    TEST_ASSERT_EQUAL(AW9523B_REG_CTL, ops[0].reg);
    TEST_ASSERT_TRUE(ops[1].is_write);
    TEST_ASSERT_EQUAL(AW9523B_REG_CTL, ops[1].reg);
    TEST_ASSERT_EQUAL((uint8_t)(0x05u | AW9523B_CTL_GPIO_MODE), ops[1].val);
}
