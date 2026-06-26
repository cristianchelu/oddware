/* Tests: spec/30-processes/auto-tare.md */

#include "unity.h"

#include "auto_tare.h"
#include "bowl_error.h"
#include "feeder_runtime.h"

void test_auto_tare_init_pending_calibration(void)
{
    auto_tare_test_reset();

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    TEST_ASSERT_FALSE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_g());
}

void test_auto_tare_on_bowl_removed_clears_drift(void)
{
    auto_tare_test_reset();
    auto_tare_anchor(12);
    auto_tare_idle_sample(12, true);
    auto_tare_idle_sample(11, true);

    TEST_ASSERT_EQUAL_INT32(1, auto_tare_drift_offset_g());
    TEST_ASSERT_EQUAL_INT32(12, auto_tare_present_grams(11, true));

    auto_tare_on_bowl_removed();

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    TEST_ASSERT_FALSE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_g());
    TEST_ASSERT_EQUAL_INT32(11, auto_tare_present_grams(11, true));
}

void test_auto_tare_initial_anchor_after_quiet_streak(void)
{
    auto_tare_test_reset();

    auto_tare_idle_sample(20, true);
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    auto_tare_idle_sample(20, true);
    auto_tare_idle_sample(20, true);
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    auto_tare_idle_sample(20, true);

    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_TRUE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(20, auto_tare_stable_grams());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_g());
}

void test_auto_tare_initial_anchor_resets_on_disturbance(void)
{
    auto_tare_test_reset();

    auto_tare_idle_sample(20, true);
    auto_tare_idle_sample(20, true);
    auto_tare_idle_sample(20, true);
    auto_tare_idle_sample(25, true);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    auto_tare_idle_sample(25, true);
    auto_tare_idle_sample(25, true);
    auto_tare_idle_sample(25, true);
    auto_tare_idle_sample(25, true);

    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_EQUAL_INT32(25, auto_tare_stable_grams());
}

void test_auto_tare_drift_compensates_slow_drift(void)
{
    auto_tare_test_reset();
    auto_tare_anchor(12);

    auto_tare_idle_sample(12, true);
    auto_tare_idle_sample(11, true);

    TEST_ASSERT_EQUAL_INT32(12, auto_tare_present_grams(11, true));
    TEST_ASSERT_EQUAL_INT32(1, auto_tare_drift_offset_g());
    TEST_ASSERT_EQUAL_INT32(12, auto_tare_stable_grams());
}

void test_auto_tare_large_delta_does_not_drift(void)
{
    auto_tare_test_reset();
    auto_tare_anchor(12);

    auto_tare_idle_sample(12, true);
    auto_tare_idle_sample(8, true);

    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_g());
    TEST_ASSERT_EQUAL_INT32(8, auto_tare_present_grams(8, true));
}

void test_auto_tare_pending_disables_drift(void)
{
    auto_tare_test_reset();

    auto_tare_idle_sample(10, true);
    auto_tare_idle_sample(9, true);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    TEST_ASSERT_EQUAL_INT32(9, auto_tare_present_grams(9, true));
}

void test_auto_tare_anchor_clears_pending(void)
{
    auto_tare_test_reset();
    auto_tare_on_bowl_removed();

    auto_tare_anchor(0);

    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_TRUE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_stable_grams());
}

void test_auto_tare_invalid_sample_resets_quiet_streak(void)
{
    auto_tare_test_reset();

    auto_tare_idle_sample(20, true);
    auto_tare_idle_sample(20, true);
    auto_tare_idle_sample(20, false);
    auto_tare_idle_sample(20, true);
    auto_tare_idle_sample(20, true);
    auto_tare_idle_sample(20, true);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
}

void test_auto_tare_dispense_active_suppresses_drift(void)
{
    auto_tare_test_reset();
    feeder_runtime_test_reset();
    auto_tare_anchor(12);

    feeder_runtime_set_dispense_active(true);
    auto_tare_idle_sample(12, true);
    auto_tare_idle_sample(11, true);
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_g());

    feeder_runtime_set_dispense_active(false);
    auto_tare_idle_sample(12, true);
    auto_tare_idle_sample(11, true);
    TEST_ASSERT_EQUAL_INT32(1, auto_tare_drift_offset_g());
    TEST_ASSERT_EQUAL_INT32(12, auto_tare_present_grams(11, true));
}

void test_auto_tare_sync_bowl_error_invalidates_on_missing(void)
{
    auto_tare_test_reset();
    auto_tare_anchor(20);

    auto_tare_sync_bowl_error(BOWL_ERROR_BOWL_MISSING);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    TEST_ASSERT_FALSE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_g());
}

void test_auto_tare_sync_bowl_error_clears_on_present(void)
{
    auto_tare_test_reset();

    auto_tare_sync_bowl_error(BOWL_ERROR_BOWL_MISSING);
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());

    auto_tare_sync_bowl_error(BOWL_ERROR_NONE);
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());

    auto_tare_idle_sample(15, true);
    auto_tare_idle_sample(15, true);
    auto_tare_idle_sample(15, true);
    auto_tare_idle_sample(15, true);

    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_EQUAL_INT32(15, auto_tare_stable_grams());
}
