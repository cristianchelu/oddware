/* Tests: spec/30-processes/scheduler-engine.md § Overfill protection */

#include "unity.h"

#include "app.h"
#include "fake_weight_port.h"
#include "feed_bowl.h"
#include "feed_config.h"
#include "weight_units.h"

void test_feed_overfill_disabled_never_skips(void)
{
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    app_test_reset();
    app_test_finish_weight_boot();
    app_bowl_dg_notify_read(WEIGHT_G_TO_DG(80), true, 1000u);

    TEST_ASSERT_FALSE(feed_overfill_should_skip_schedule(1000u));
}

void test_feed_overfill_skips_when_bowl_at_threshold(void)
{
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    app_test_reset();
    app_test_finish_weight_boot();
    app_bowl_dg_notify_read(WEIGHT_G_TO_DG(50), true, 1000u);

    TEST_ASSERT_TRUE(feed_config_overfill_enabled_set(true));
    TEST_ASSERT_TRUE(feed_config_overfill_threshold_g_set(50u));

    TEST_ASSERT_TRUE(feed_overfill_should_skip_schedule(1000u));
}

void test_feed_overfill_dispenses_when_bowl_below_threshold(void)
{
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    app_test_reset();
    app_test_finish_weight_boot();
    app_bowl_dg_notify_read(WEIGHT_G_TO_DG(49), true, 1000u);

    TEST_ASSERT_TRUE(feed_config_overfill_enabled_set(true));
    TEST_ASSERT_TRUE(feed_config_overfill_threshold_g_set(50u));

    TEST_ASSERT_FALSE(feed_overfill_should_skip_schedule(1000u));
}

void test_feed_overfill_unknown_bowl_does_not_skip(void)
{
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    app_test_reset();
    app_test_finish_weight_boot();

    TEST_ASSERT_TRUE(feed_config_overfill_enabled_set(true));
    TEST_ASSERT_TRUE(feed_config_overfill_threshold_g_set(50u));

    TEST_ASSERT_FALSE(feed_overfill_should_skip_schedule(1000u));
}

void test_feed_bowl_known_g_returns_display_grams(void)
{
    uint16_t grams = 0u;

    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    app_test_reset();
    app_test_finish_weight_boot();
    app_bowl_dg_notify_read(WEIGHT_G_TO_DG(42), true, 1000u);

    TEST_ASSERT_TRUE(feed_bowl_known_g(1000u, &grams));
    TEST_ASSERT_EQUAL_UINT16(42u, grams);
}
