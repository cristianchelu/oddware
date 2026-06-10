/* Tests: spec/30-processes/weighing.md § host calibration */

#include <string.h>

#include "unity.h"

#include "config_keys.h"
#include "fake_config_port.h"
#include "weigh_cal.h"
#include "weigh_product.h"

void test_weigh_cal_apply_linear(void)
{
    weigh_cal_model_t model = {
        .zero_raw = 100,
        .span_g = 500,
        .span_raw = 600,
        .zero_set = true,
        .span_set = true,
    };
    int32_t grams;

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_apply(&model, 100, &grams));
    TEST_ASSERT_EQUAL(0, grams);

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_apply(&model, 600, &grams));
    TEST_ASSERT_EQUAL(500, grams);

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_apply(&model, 350, &grams));
    TEST_ASSERT_EQUAL(250, grams);
}

void test_weigh_cal_food_grams_subtracts_bowl(void)
{
    weigh_cal_model_t model = {
        .zero_raw = 1000,
        .span_g = WEIGH_BOWL_MASS_G,
        .span_raw = 1500,
        .zero_set = true,
        .span_set = true,
    };
    int32_t food;

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_food_grams(&model, 1500, &food));
    TEST_ASSERT_EQUAL(0, food);

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_food_grams(&model, 1650, &food));
    TEST_ASSERT_EQUAL(105, food);
}

void test_weigh_cal_save_zero_clears_span(void)
{
    const config_port_t *cfg = fake_config_port_get();
    weigh_cal_model_t model;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_save_zero(cfg, 42, &model));
    TEST_ASSERT_TRUE(model.zero_set);
    TEST_ASSERT_FALSE(model.span_set);

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_save_span(cfg, WEIGH_BOWL_MASS_G, 242, &model));
    TEST_ASSERT_TRUE(weigh_cal_is_complete(&model));

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_save_zero(cfg, 50, &model));
    TEST_ASSERT_FALSE(weigh_cal_is_complete(&model));
    TEST_ASSERT_TRUE(weigh_cal_zero_pending_span(&model));
}

void test_weigh_cal_load_round_trip(void)
{
    const config_port_t *cfg = fake_config_port_get();
    weigh_cal_model_t saved;
    weigh_cal_model_t loaded;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_save_zero(cfg, 100, &saved));
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_save_span(cfg, WEIGH_BOWL_MASS_G, 400, &saved));

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_load(cfg, &loaded));
    TEST_ASSERT_TRUE(weigh_cal_is_complete(&loaded));
    TEST_ASSERT_EQUAL(100, loaded.zero_raw);
    TEST_ASSERT_EQUAL(WEIGH_BOWL_MASS_G, loaded.span_g);
    TEST_ASSERT_EQUAL(400, loaded.span_raw);
}

void test_weigh_cal_save_span_requires_zero(void)
{
    const config_port_t *cfg = fake_config_port_get();
    weigh_cal_model_t model;

    fake_config_port_reset();
    memset(&model, 0, sizeof(model));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, weigh_cal_save_span(cfg, WEIGH_BOWL_MASS_G, 300, &model));
}

void test_weigh_cal_load_zero_only_pending_span(void)
{
    const config_port_t *cfg = fake_config_port_get();
    weigh_cal_model_t loaded;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_CALIB, CONFIG_KEY_CALIB_ZERO, "42"));
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_load(cfg, &loaded));
    TEST_ASSERT_TRUE(weigh_cal_zero_pending_span(&loaded));
    TEST_ASSERT_FALSE(weigh_cal_is_complete(&loaded));
}

void test_weigh_cal_load_span_g_without_span_raw(void)
{
    const config_port_t *cfg = fake_config_port_get();
    weigh_cal_model_t loaded;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_CALIB, CONFIG_KEY_CALIB_ZERO, "10"));
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_CALIB, CONFIG_KEY_CALIB_SPAN_G, "350"));
    TEST_ASSERT_EQUAL(PORT_ERR_IO, weigh_cal_load(cfg, &loaded));
    TEST_ASSERT_FALSE(weigh_cal_is_complete(&loaded));
}

void test_weigh_cal_load_span_without_zero(void)
{
    const config_port_t *cfg = fake_config_port_get();
    weigh_cal_model_t loaded;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_CALIB, CONFIG_KEY_CALIB_SPAN_G, "350"));
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_CALIB, CONFIG_KEY_CALIB_SPAN_RAW, "400"));
    TEST_ASSERT_EQUAL(PORT_ERR_IO, weigh_cal_load(cfg, &loaded));
    TEST_ASSERT_FALSE(loaded.span_set);
}

void test_weigh_cal_clear(void)
{
    const config_port_t *cfg = fake_config_port_get();
    weigh_cal_model_t model;
    weigh_cal_model_t loaded;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_save_zero(cfg, 10, &model));
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_save_span(cfg, WEIGH_BOWL_MASS_G, 110, &model));
    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_clear(cfg, &model));
    TEST_ASSERT_FALSE(weigh_cal_is_complete(&model));

    TEST_ASSERT_EQUAL(PORT_OK, weigh_cal_load(cfg, &loaded));
    TEST_ASSERT_FALSE(weigh_cal_is_complete(&loaded));
}
