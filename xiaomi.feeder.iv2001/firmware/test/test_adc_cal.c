/* Tests: spec/30-processes/battery-monitoring.md, uart-console.md § adc cal */

#include <string.h>

#include "unity.h"

#include "adc_cal.h"
#include "adc_driver.h"
#include "board_gpio_iv2001.h"
#include "config_keys.h"
#include "fake_config_port.h"
#include "fake_gpio_expander_port.h"

void test_adc_cal_apply_default_11_to_1(void)
{
    adc_cal_model_t model;

    adc_cal_reset(NULL, &model);
    TEST_ASSERT_EQUAL_UINT16(6600u, adc_cal_apply_pin_mv(&model, 600u));
}

void test_adc_cal_capture_stores_scale_x1000(void)
{
    adc_cal_model_t model;

    TEST_ASSERT_EQUAL(PORT_OK, adc_cal_capture(NULL, 6385u, 600u, &model));
    TEST_ASSERT_EQUAL_UINT32(10642u, model.scale_x1000);
    TEST_ASSERT_TRUE(model.customized);
    TEST_ASSERT_EQUAL_UINT16(6385u, adc_cal_apply_pin_mv(&model, 600u));
}

void test_adc_cal_load_default_when_key_missing(void)
{
    const config_port_t *cfg = fake_config_port_get();
    adc_cal_model_t model;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, adc_cal_load(cfg, &model));
    TEST_ASSERT_FALSE(model.customized);
    TEST_ASSERT_EQUAL_UINT32(11000u, model.scale_x1000);
}

void test_adc_cal_load_round_trip(void)
{
    const config_port_t *cfg = fake_config_port_get();
    adc_cal_model_t loaded;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_POWER,
                                 CONFIG_KEY_POWER_BATT_SCALE_X1000,
                                 "10642"));
    TEST_ASSERT_EQUAL(PORT_OK, adc_cal_load(cfg, &loaded));
    TEST_ASSERT_TRUE(loaded.customized);
    TEST_ASSERT_EQUAL_UINT32(10642u, loaded.scale_x1000);
}

void test_adc_cal_reset_clears_key(void)
{
    const config_port_t *cfg = fake_config_port_get();
    adc_cal_model_t model;
    adc_cal_model_t loaded;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, adc_cal_capture(cfg, 6385u, 600u, &model));
    TEST_ASSERT_EQUAL(PORT_OK, adc_cal_reset(cfg, &model));
    TEST_ASSERT_FALSE(model.customized);
    TEST_ASSERT_EQUAL(PORT_OK, adc_cal_load(cfg, &loaded));
    TEST_ASSERT_FALSE(loaded.customized);
}

void test_adc_cal_capture_rejects_low_true_mv(void)
{
    adc_cal_model_t model;

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, adc_cal_capture(NULL, 2500u, 600u, &model));
}

void test_adc_cal_capture_rejects_ratio_out_of_range(void)
{
    adc_cal_model_t model;

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, adc_cal_capture(NULL, 5000u, 1000u, &model));
}

void test_adc_driver_init_loads_cal_from_nvdm(void)
{
    const config_port_t *cfg = fake_config_port_get();
    adc_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .config = cfg,
        .read_raw = NULL,
        .delay_ms = NULL,
    };
    adc_driver_state_t state;
    adc_cal_status_t status;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_POWER,
                                 CONFIG_KEY_POWER_BATT_SCALE_X1000,
                                 "10642"));
    adc_driver_init(&state, &hw);
    adc_driver_cal_status(&state, &status);
    TEST_ASSERT_TRUE(status.customized);
    TEST_ASSERT_EQUAL_UINT32(10642u, status.scale_x1000);
}

void test_adc_driver_init_invalid_nvdm_uses_default(void)
{
    const config_port_t *cfg = fake_config_port_get();
    adc_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .config = cfg,
        .read_raw = NULL,
        .delay_ms = NULL,
    };
    adc_driver_state_t state;
    adc_cal_status_t status;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_POWER,
                                 CONFIG_KEY_POWER_BATT_SCALE_X1000,
                                 "1"));
    adc_driver_init(&state, &hw);
    adc_driver_cal_status(&state, &status);
    TEST_ASSERT_FALSE(status.customized);
    TEST_ASSERT_EQUAL_UINT32(11000u, status.scale_x1000);
}
