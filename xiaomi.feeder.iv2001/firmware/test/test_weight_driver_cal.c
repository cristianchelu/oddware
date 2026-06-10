/* Tests: spec/30-processes/weighing.md */

#include "unity.h"

#include "board_gpio_iv2001.h"
#include "cs1270.h"
#include "fake_config_port.h"
#include "fake_gpio_expander_port.h"
#include "fake_time.h"
#include "weigh_cal.h"
#include "weigh_product.h"
#include "weight_driver.h"

static port_err_t s_exchange_err;
static uint8_t s_rsp[CS1270_FRAME_LEN];
static bool s_has_rsp;
static uint32_t s_exchange_count;
static uint32_t s_warm_remaining;
static uint32_t s_fail_remaining;
static uint32_t s_delay_total;
static int32_t s_weight_after_warm;
static bool s_use_weight_after_warm;

static void set_weight_rsp(int32_t grams)
{
    uint16_t mag;
    uint8_t cmd3;

    if (grams < 0) {
        cmd3 = 0x01u;
        mag = (uint16_t)(-grams);
    } else {
        cmd3 = 0x00u;
        mag = (uint16_t)grams;
    }

    s_rsp[0] = 0xB2u;
    s_rsp[1] = 0xA5u;
    s_rsp[2] = cmd3;
    s_rsp[3] = (uint8_t)(mag >> 8);
    s_rsp[4] = (uint8_t)(mag & 0xFFu);
    s_rsp[5] = cs1270_checksum_rsp(cmd3, s_rsp[3], s_rsp[4]);
    s_has_rsp = true;
    s_exchange_err = PORT_OK;
}

static void set_warming_rsp(void)
{
    s_rsp[0] = 0xB2u;
    s_rsp[1] = 0xA5u;
    s_rsp[2] = 0x0Fu;
    s_rsp[3] = 0xFFu;
    s_rsp[4] = 0x88u;
    s_rsp[5] = cs1270_checksum_rsp(0x0Fu, 0xFFu, 0x88u);
    s_has_rsp = true;
    s_exchange_err = PORT_OK;
}

static port_err_t fake_exchange(const uint8_t tx[CS1270_FRAME_LEN],
                                uint8_t rx[CS1270_FRAME_LEN],
                                uint32_t timeout_ms)
{
    (void)tx;
    (void)timeout_ms;

    s_exchange_count++;
    if (s_fail_remaining > 0u) {
        s_fail_remaining--;
        return PORT_ERR_IO;
    }

    if (s_warm_remaining > 0u) {
        s_warm_remaining--;
        set_warming_rsp();
    } else if (s_use_weight_after_warm) {
        set_weight_rsp(s_weight_after_warm);
    }

    if (s_exchange_err != PORT_OK || !s_has_rsp) {
        return s_exchange_err != PORT_OK ? s_exchange_err : PORT_ERR_IO;
    }

    for (uint8_t i = 0u; i < CS1270_FRAME_LEN; i++) {
        rx[i] = s_rsp[i];
    }
    return PORT_OK;
}

static void driver_delay(uint32_t ms)
{
    s_delay_total += ms;
    fake_time_advance_ms(ms);
}

static cs1270_uart_ops_t s_uart_ops = {
    .exchange = fake_exchange,
    .delay_ms = driver_delay,
};

static weight_driver_state_t s_state;

static void driver_reset(void)
{
    weight_hw_t hw = {
        .expander = NULL,
        .uart = &s_uart_ops,
        .config = fake_config_port_get(),
        .delay_ms = driver_delay,
    };

    fake_config_port_reset();
    fake_time_reset();
    s_has_rsp = false;
    s_exchange_err = PORT_OK;
    s_exchange_count = 0u;
    s_warm_remaining = 0u;
    s_fail_remaining = 0u;
    s_delay_total = 0u;
    s_use_weight_after_warm = false;
    weight_driver_init(&s_state, &hw);
    s_state.powered = true;
    s_state.boot_done = true;
}

void test_weight_power_on_preserves_display_rail(void)
{
    const gpio_expander_port_t *exp = fake_gpio_expander_port_get();
    weight_hw_t hw = {
        .expander = exp,
        .uart = &s_uart_ops,
        .config = fake_config_port_get(),
        .delay_ms = driver_delay,
    };

    fake_gpio_expander_reset();
    TEST_ASSERT_EQUAL(PORT_OK, exp->set_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                            BOARD_GPIO_DISPLAY_RAIL_PIN,
                                            true));
    weight_driver_init(&s_state, &hw);
    TEST_ASSERT_EQUAL(PORT_OK, weight_rail_enable(&s_state));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                            BOARD_GPIO_DISPLAY_RAIL_PIN));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_CS1270_PWR_PORT,
                                            BOARD_GPIO_CS1270_PWR_PIN));
}

void test_weight_boot_settle_runs_once(void)
{
    weight_hw_t hw = {
        .expander = NULL,
        .uart = &s_uart_ops,
        .config = fake_config_port_get(),
        .delay_ms = driver_delay,
    };

    fake_time_reset();
    s_delay_total = 0u;
    weight_driver_init(&s_state, &hw);
    s_state.powered = true;
    TEST_ASSERT_EQUAL(PORT_OK, weight_boot_settle(&s_state));
    TEST_ASSERT_EQUAL(CS1270_BOOT_MS, s_delay_total);
    TEST_ASSERT_EQUAL(PORT_OK, weight_boot_settle(&s_state));
    TEST_ASSERT_EQUAL(CS1270_BOOT_MS, s_delay_total);
}

void test_weight_driver_read_requires_host_cal(void)
{
    int32_t grams;

    driver_reset();
    set_weight_rsp(100);
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_SUPPORTED, weight_read_grams(&s_state, &grams));
    TEST_ASSERT_EQUAL(PORT_OK, weight_read_raw_grams(&s_state, &grams));
    TEST_ASSERT_EQUAL(100, grams);
}

void test_weight_driver_read_requires_boot(void)
{
    int32_t grams;
    weigh_cal_model_t model = {
        .zero_raw = 1000,
        .span_g = WEIGH_BOWL_MASS_G,
        .span_raw = 1500,
        .zero_set = true,
        .span_set = true,
    };

    driver_reset();
    s_state.boot_done = false;
    s_state.cal = model;
    set_weight_rsp(1500);
    TEST_ASSERT_EQUAL(PORT_ERR_IO, weight_read_grams(&s_state, &grams));
}

void test_weight_driver_cal_span_then_read_empty_bowl(void)
{
    int32_t grams;

    driver_reset();
    set_weight_rsp(1000);
    TEST_ASSERT_EQUAL(PORT_OK, weight_calibrate_zero(&s_state));

    set_weight_rsp(1500);
    TEST_ASSERT_EQUAL(PORT_OK, weight_calibrate_span(&s_state));

    set_weight_rsp(1500);
    TEST_ASSERT_EQUAL(PORT_OK, weight_read_grams(&s_state, &grams));
    TEST_ASSERT_EQUAL(0, grams);
}

void test_weight_driver_read_food_with_bowl_and_contents(void)
{
    int32_t grams;

    driver_reset();
    set_weight_rsp(1000);
    TEST_ASSERT_EQUAL(PORT_OK, weight_calibrate_zero(&s_state));
    set_weight_rsp(1500);
    TEST_ASSERT_EQUAL(PORT_OK, weight_calibrate_span(&s_state));

    set_weight_rsp(1650);
    TEST_ASSERT_EQUAL(PORT_OK, weight_read_grams(&s_state, &grams));
    TEST_ASSERT_EQUAL(105, grams);
}

void test_weight_driver_cal_status(void)
{
    driver_reset();
    TEST_ASSERT_EQUAL(WEIGHT_CAL_UNCALIBRATED, weight_driver_cal_status(&s_state));

    set_weight_rsp(1000);
    TEST_ASSERT_EQUAL(PORT_OK, weight_calibrate_zero(&s_state));
    TEST_ASSERT_EQUAL(WEIGHT_CAL_CAPTURING_SPAN, weight_driver_cal_status(&s_state));

    set_weight_rsp(1500);
    TEST_ASSERT_EQUAL(PORT_OK, weight_calibrate_span(&s_state));
    TEST_ASSERT_EQUAL(WEIGHT_CAL_SUCCESS, weight_driver_cal_status(&s_state));
}

void test_weight_driver_span_uses_bowl_mass(void)
{
    weigh_cal_model_t loaded;

    driver_reset();
    set_weight_rsp(1000);
    TEST_ASSERT_EQUAL(PORT_OK, weight_calibrate_zero(&s_state));
    set_weight_rsp(1500);
    TEST_ASSERT_EQUAL(PORT_OK, weight_calibrate_span(&s_state));

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_load(fake_config_port_get(), &loaded));
    TEST_ASSERT_EQUAL(WEIGH_BOWL_MASS_G, loaded.span_g);
}

void test_weight_driver_calibrate_span_without_zero(void)
{
    driver_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, weight_calibrate_span(&s_state));
}

void test_weight_driver_polls_through_boot_warming(void)
{
    int32_t grams;
    weigh_cal_model_t model = {
        .zero_raw = 1000,
        .span_g = WEIGH_BOWL_MASS_G,
        .span_raw = 1500,
        .zero_set = true,
        .span_set = true,
    };

    driver_reset();
    s_state.cal = model;
    s_exchange_count = 0u;
    s_delay_total = 0u;
    s_warm_remaining = 2u;
    s_weight_after_warm = 1500;
    s_use_weight_after_warm = true;

    TEST_ASSERT_EQUAL(PORT_OK, weight_read_grams(&s_state, &grams));
    TEST_ASSERT_EQUAL(0, grams);
    TEST_ASSERT_EQUAL(3u, s_exchange_count);
    TEST_ASSERT_TRUE(s_delay_total >= 2u * CS1270_POLL_MS);
}

void test_weight_driver_uart_retry_succeeds_after_failures(void)
{
    int32_t grams;
    weigh_cal_model_t model = {
        .zero_raw = 1000,
        .span_g = WEIGH_BOWL_MASS_G,
        .span_raw = 1500,
        .zero_set = true,
        .span_set = true,
    };

    driver_reset();
    s_state.cal = model;
    s_exchange_count = 0u;
    s_delay_total = 0u;
    s_fail_remaining = 2u;
    set_weight_rsp(1500);

    TEST_ASSERT_EQUAL(PORT_OK, weight_read_grams(&s_state, &grams));
    TEST_ASSERT_EQUAL(0, grams);
    TEST_ASSERT_EQUAL(3u, s_exchange_count);
    TEST_ASSERT_TRUE(s_delay_total >= 2u * CS1270_UART_RETRY_MS);
}

void test_weight_driver_read_rejects_stuck_warming(void)
{
    int32_t grams;
    weigh_cal_model_t model = {
        .zero_raw = 1000,
        .span_g = WEIGH_BOWL_MASS_G,
        .span_raw = 1500,
        .zero_set = true,
        .span_set = true,
    };

    driver_reset();
    s_state.cal = model;
    set_warming_rsp();
    s_warm_remaining = CS1270_WARM_POLL_MAX;

    TEST_ASSERT_EQUAL(PORT_ERR_IO, weight_read_grams(&s_state, &grams));
}
