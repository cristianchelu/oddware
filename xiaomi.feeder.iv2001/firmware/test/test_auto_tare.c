/* Tests: spec/30-processes/auto-tare.md */

#include "unity.h"

#include "auto_tare.h"
#include "bowl_error.h"
#include "feeder_runtime.h"
#include "weight_units.h"

void test_auto_tare_init_pending_calibration(void)
{
    auto_tare_test_reset();

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    TEST_ASSERT_FALSE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_dg());
}

void test_auto_tare_on_bowl_removed_clears_drift(void)
{
    auto_tare_test_reset();
    auto_tare_anchor(120);
    auto_tare_idle_sample(120, true);
    auto_tare_idle_sample(119, true);

    TEST_ASSERT_EQUAL_INT32(1, auto_tare_drift_offset_dg());
    TEST_ASSERT_EQUAL_INT32(120, auto_tare_present_dg(119, true));

    auto_tare_on_bowl_removed();

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    TEST_ASSERT_FALSE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_dg());
    TEST_ASSERT_EQUAL_INT32(119, auto_tare_present_dg(119, true));
}

void test_auto_tare_initial_anchor_after_quiet_streak(void)
{
    auto_tare_test_reset();

    auto_tare_idle_sample(200, true);
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(200, true);
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    auto_tare_idle_sample(200, true);

    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_TRUE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(200, auto_tare_stable_dg());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_dg());
}

void test_auto_tare_initial_anchor_resets_on_disturbance(void)
{
    auto_tare_test_reset();

    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(250, true);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    auto_tare_idle_sample(250, true);
    auto_tare_idle_sample(250, true);
    auto_tare_idle_sample(250, true);
    auto_tare_idle_sample(250, true);

    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_EQUAL_INT32(250, auto_tare_stable_dg());
}

void test_auto_tare_initial_anchor_resets_on_one_gram_jump(void)
{
    auto_tare_test_reset();

    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(210, true);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    auto_tare_idle_sample(210, true);
    auto_tare_idle_sample(210, true);
    auto_tare_idle_sample(210, true);
    auto_tare_idle_sample(210, true);

    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_EQUAL_INT32(210, auto_tare_stable_dg());
}

void test_auto_tare_drift_compensates_slow_drift(void)
{
    auto_tare_test_reset();
    auto_tare_anchor(120);

    auto_tare_idle_sample(120, true);
    auto_tare_idle_sample(119, true);

    TEST_ASSERT_EQUAL_INT32(120, auto_tare_present_dg(119, true));
    TEST_ASSERT_EQUAL_INT32(1, auto_tare_drift_offset_dg());
    TEST_ASSERT_EQUAL_INT32(120, auto_tare_stable_dg());
}

void test_auto_tare_large_delta_does_not_drift(void)
{
    auto_tare_test_reset();
    auto_tare_anchor(120);

    auto_tare_idle_sample(120, true);
    auto_tare_idle_sample(80, true);

    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_dg());
    TEST_ASSERT_EQUAL_INT32(80, auto_tare_present_dg(80, true));
}

void test_auto_tare_pending_disables_drift(void)
{
    auto_tare_test_reset();

    auto_tare_idle_sample(100, true);
    auto_tare_idle_sample(90, true);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    TEST_ASSERT_EQUAL_INT32(90, auto_tare_present_dg(90, true));
}

void test_auto_tare_anchor_clears_pending(void)
{
    auto_tare_test_reset();
    auto_tare_on_bowl_removed();

    auto_tare_anchor(0);

    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_TRUE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_stable_dg());
}

void test_auto_tare_invalid_sample_resets_quiet_streak(void)
{
    auto_tare_test_reset();

    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(200, false);
    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(200, true);
    auto_tare_idle_sample(200, true);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
}

void test_auto_tare_dispense_active_suppresses_drift(void)
{
    auto_tare_test_reset();
    feeder_runtime_test_reset();
    auto_tare_anchor(120);

    feeder_runtime_set_dispense_active(true);
    auto_tare_idle_sample(120, true);
    auto_tare_idle_sample(119, true);
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_dg());

    feeder_runtime_set_dispense_active(false);
    auto_tare_idle_sample(120, true);
    auto_tare_idle_sample(119, true);
    TEST_ASSERT_EQUAL_INT32(1, auto_tare_drift_offset_dg());
    TEST_ASSERT_EQUAL_INT32(120, auto_tare_present_dg(119, true));
}

void test_auto_tare_sync_bowl_error_invalidates_on_missing(void)
{
    auto_tare_test_reset();
    auto_tare_anchor(200);

    auto_tare_sync_bowl_error(BOWL_ERROR_BOWL_MISSING);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    TEST_ASSERT_FALSE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_drift_offset_dg());
}

void test_auto_tare_sync_bowl_error_clears_on_present(void)
{
    auto_tare_test_reset();

    auto_tare_sync_bowl_error(BOWL_ERROR_BOWL_MISSING);
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());

    auto_tare_sync_bowl_error(BOWL_ERROR_NONE);
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());

    auto_tare_idle_sample(150, true);
    auto_tare_idle_sample(150, true);
    auto_tare_idle_sample(150, true);
    auto_tare_idle_sample(150, true);

    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_EQUAL_INT32(150, auto_tare_stable_dg());
}
