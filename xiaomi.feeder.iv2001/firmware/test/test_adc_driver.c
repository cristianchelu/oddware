/* Tests: spec/30-processes/battery-monitoring.md, jam-detection.md § ADC path */

#include "unity.h"

#include "adc_driver.h"
#include "adc_limits.h"
#include "board_gpio_iv2001.h"
#include "config_keys.h"
#include "fake_config_port.h"
#include "fake_gpio_expander_port.h"
#include "fake_time.h"

static uint16_t s_raw_queue[16];
static uint8_t s_raw_queue_len;
static uint8_t s_raw_queue_idx;
static port_err_t s_read_raw_err;

static port_err_t test_read_raw(uint16_t *raw)
{
    if (raw == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_read_raw_err != PORT_OK) {
        return s_read_raw_err;
    }

    if (s_raw_queue_idx >= s_raw_queue_len) {
        return PORT_ERR_IO;
    }

    *raw = s_raw_queue[s_raw_queue_idx++];
    return PORT_OK;
}

static void test_delay_ms(uint32_t ms)
{
    fake_time_advance_ms(ms);
}

static adc_driver_state_t s_state;

static void adc_driver_test_reset(void)
{
    adc_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .config = fake_config_port_get(),
        .read_raw = test_read_raw,
        .delay_ms = test_delay_ms,
    };

    const gpio_expander_port_t *exp = fake_gpio_expander_port_get();

    fake_config_port_reset();
    fake_gpio_expander_reset();
    fake_gpio_expander_set_inputs(0u, 0u);
    fake_time_reset();
    s_raw_queue_len = 0u;
    s_raw_queue_idx = 0u;
    s_read_raw_err = PORT_OK;
    TEST_ASSERT_EQUAL(PORT_OK, exp->reset());
    TEST_ASSERT_EQUAL(PORT_OK,
                      exp->configure(BOARD_GPIO_BOOT_DIR_P0,
                                     BOARD_GPIO_BOOT_DIR_P1,
                                     BOARD_GPIO_BOOT_OUT_P0,
                                     BOARD_GPIO_BOOT_OUT_P1));
    adc_driver_init(&s_state, &hw);
}

static void adc_test_set_raw_sequence(const uint16_t *raws, uint8_t count)
{
    uint8_t i;

    s_raw_queue_len = count;
    s_raw_queue_idx = 0u;
    for (i = 0u; i < count; i++) {
        s_raw_queue[i] = raws[i];
    }
}

void test_adc_raw_to_mv_converts_2048_to_1250(void)
{
    TEST_ASSERT_EQUAL_UINT16(1250u, adc_driver_raw_to_mv(2048u));
}

void test_adc_motor_load_selects_mux_settles_and_samples_once(void)
{
    uint16_t ma = 0u;

    adc_driver_test_reset();
    adc_test_set_raw_sequence((const uint16_t[]){ 2048u }, 1u);

    TEST_ASSERT_EQUAL(PORT_OK, adc_driver_read_motor_load_ma(&s_state, &ma));
    TEST_ASSERT_EQUAL_UINT16(1250u, ma);
    TEST_ASSERT_EQUAL(ADC_MUX_SETTLE_MS, (unsigned)fake_time_ticks());
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_ADC_MUX_PORT,
                                             BOARD_GPIO_ADC_MUX_PIN));
    TEST_ASSERT_EQUAL(1u, fake_gpio_expander_set_pin_calls());
}

void test_adc_motor_load_idempotent_mux_low(void)
{
    uint16_t ma = 0u;

    adc_driver_test_reset();
    adc_test_set_raw_sequence((const uint16_t[]){ 1000u, 1000u }, 2u);

    TEST_ASSERT_EQUAL(PORT_OK, adc_driver_read_motor_load_ma(&s_state, &ma));
    TEST_ASSERT_EQUAL(PORT_OK, adc_driver_read_motor_load_ma(&s_state, &ma));
    TEST_ASSERT_EQUAL(2u, fake_gpio_expander_set_pin_calls());
}

/* Regression: motor EN exclusivity uses output latch, not input pad (FAULT shares EN). */
void test_adc_battery_rejects_when_motor_en_high(void)
{
    uint16_t mv = 0u;

    const gpio_expander_port_t *exp = fake_gpio_expander_port_get();

    adc_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      exp->configure(BOARD_GPIO_BOOT_DIR_P0,
                                     BOARD_GPIO_BOOT_DIR_P1,
                                     BOARD_GPIO_MOTOR_EN_MASK,
                                     BOARD_GPIO_BOOT_OUT_P1));

    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, adc_driver_read_battery_mv(&s_state, &mv));
    TEST_ASSERT_EQUAL(0u, fake_gpio_expander_set_pin_calls());
}

void test_adc_battery_averages_ten_samples_and_restores_mux(void)
{
    uint16_t mv = 0u;
    uint16_t raws[ADC_BATTERY_SAMPLE_CNT];
    uint8_t i;

    adc_driver_test_reset();
    for (i = 0u; i < ADC_BATTERY_SAMPLE_CNT; i++) {
        raws[i] = 2048u;
    }
    adc_test_set_raw_sequence(raws, ADC_BATTERY_SAMPLE_CNT);

    TEST_ASSERT_EQUAL(PORT_OK, adc_driver_read_battery_mv(&s_state, &mv));
    TEST_ASSERT_EQUAL_UINT16(13750u, mv);
    TEST_ASSERT_EQUAL(ADC_MUX_SETTLE_MS, (unsigned)fake_time_ticks());
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_ADC_MUX_PORT,
                                             BOARD_GPIO_ADC_MUX_PIN));
    TEST_ASSERT_EQUAL(2u, fake_gpio_expander_set_pin_calls());
}

void test_adc_battery_mux_call_order(void)
{
    uint16_t mv = 0u;
    uint8_t port;
    uint8_t pin;
    bool level;

    adc_driver_test_reset();
    adc_test_set_raw_sequence((const uint16_t[]){ 2048u, 2048u, 2048u, 2048u,
                                                  2048u, 2048u, 2048u, 2048u,
                                                  2048u, 2048u },
                              ADC_BATTERY_SAMPLE_CNT);

    TEST_ASSERT_EQUAL(PORT_OK, adc_driver_read_battery_mv(&s_state, &mv));

    TEST_ASSERT_TRUE(fake_gpio_expander_get_set_pin_log(0u, &port, &pin, &level));
    TEST_ASSERT_EQUAL(BOARD_GPIO_ADC_MUX_PORT, port);
    TEST_ASSERT_EQUAL(BOARD_GPIO_ADC_MUX_PIN, pin);
    TEST_ASSERT_TRUE(level);

    TEST_ASSERT_TRUE(fake_gpio_expander_get_set_pin_log(1u, &port, &pin, &level));
    TEST_ASSERT_EQUAL(BOARD_GPIO_ADC_MUX_PORT, port);
    TEST_ASSERT_EQUAL(BOARD_GPIO_ADC_MUX_PIN, pin);
    TEST_ASSERT_FALSE(level);
}

void test_adc_battery_restores_mux_on_read_error(void)
{
    uint16_t mv = 0u;
    uint16_t raws[5];

    adc_driver_test_reset();
    adc_test_set_raw_sequence(raws, 5u);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, adc_driver_read_battery_mv(&s_state, &mv));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_ADC_MUX_PORT,
                                             BOARD_GPIO_ADC_MUX_PIN));
    TEST_ASSERT_EQUAL(2u, fake_gpio_expander_set_pin_calls());
}

void test_adc_battery_rejects_null_mv(void)
{
    adc_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      adc_driver_read_battery_mv(&s_state, NULL));
}

void test_adc_motor_load_rejects_null_ma(void)
{
    adc_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      adc_driver_read_motor_load_ma(&s_state, NULL));
}

void test_adc_driver_cal_capture_updates_state(void)
{
    adc_cal_status_t status;
    uint16_t raws[ADC_BATTERY_SAMPLE_CNT];
    uint8_t i;

    adc_driver_test_reset();
    for (i = 0u; i < ADC_BATTERY_SAMPLE_CNT; i++) {
        raws[i] = 983u;
    }
    adc_test_set_raw_sequence(raws, ADC_BATTERY_SAMPLE_CNT);

    TEST_ASSERT_EQUAL(PORT_OK, adc_driver_cal_capture(&s_state, 6385u));
    adc_driver_cal_status(&s_state, &status);
    TEST_ASSERT_TRUE(status.customized);
    TEST_ASSERT_EQUAL_UINT32(10642u, status.scale_x1000);
}

void test_adc_driver_init_null_safe(void)
{
    adc_driver_init(NULL, NULL);
    adc_driver_init(&s_state, NULL);
}
