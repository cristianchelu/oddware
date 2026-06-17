/* Tests: spec/30-processes/weighing.md, mqtt-protocol.md § Bowl weight */

#include "unity.h"

#include "bowl_grams_present.h"
#include "weigh_product.h"
#include "weight_port.h"

void test_bowl_grams_present_uncalibrated_is_unknown(void)
{
    int32_t g = 99;

    TEST_ASSERT_EQUAL(BOWL_GRAMS_UNKNOWN,
                      bowl_grams_present(WEIGHT_CAL_UNCALIBRATED, true, 42, &g));
    TEST_ASSERT_EQUAL(0, g);
}

void test_bowl_grams_present_bowl_missing_is_unknown(void)
{
    int32_t g = 0;

    TEST_ASSERT_EQUAL(BOWL_GRAMS_UNKNOWN,
                      bowl_grams_present(WEIGHT_CAL_SUCCESS,
                                         true,
                                         -(int32_t)WEIGH_BOWL_MISSING_THRESHOLD_G - 1,
                                         &g));
}

void test_bowl_grams_present_no_sample_is_unknown(void)
{
    int32_t g = 0;

    TEST_ASSERT_EQUAL(BOWL_GRAMS_UNKNOWN,
                      bowl_grams_present(WEIGHT_CAL_SUCCESS, false, 42, &g));
}

void test_bowl_grams_present_implausible_high_is_unknown(void)
{
    TEST_ASSERT_EQUAL(BOWL_GRAMS_UNKNOWN,
                      bowl_grams_present(WEIGHT_CAL_SUCCESS,
                                         true,
                                         BOWL_GRAMS_IMPLAUSIBLE_HIGH_G + 1,
                                         NULL));
}

void test_bowl_grams_present_negative_drift_is_zero(void)
{
    int32_t g = -1;

    TEST_ASSERT_EQUAL(BOWL_GRAMS_KNOWN,
                      bowl_grams_present(WEIGHT_CAL_SUCCESS, true, -1, &g));
    TEST_ASSERT_EQUAL(0, g);
}

void test_bowl_grams_present_known_positive(void)
{
    int32_t g = 0;

    TEST_ASSERT_EQUAL(BOWL_GRAMS_KNOWN,
                      bowl_grams_present(WEIGHT_CAL_SUCCESS, true, 42, &g));
    TEST_ASSERT_EQUAL(42, g);
}

void test_bowl_grams_present_full_range_not_capped(void)
{
    int32_t g = 0;

    TEST_ASSERT_EQUAL(BOWL_GRAMS_KNOWN,
                      bowl_grams_present(WEIGHT_CAL_SUCCESS, true, 1500, &g));
    TEST_ASSERT_EQUAL(1500, g);
}

void test_bowl_grams_display_digits_uncalibrated_dash(void)
{
    uint16_t shown = 99u;

    TEST_ASSERT_EQUAL(BOWL_DISPLAY_DASH,
                      bowl_grams_display_digits(WEIGHT_CAL_UNCALIBRATED, true, 42, &shown));
}

void test_bowl_grams_display_digits_underflow(void)
{
    uint16_t shown = 0u;

    TEST_ASSERT_EQUAL(BOWL_DISPLAY_UNDERFLOW,
                      bowl_grams_display_digits(WEIGHT_CAL_SUCCESS,
                                                true,
                                                -(int32_t)WEIGH_BOWL_MISSING_THRESHOLD_G - 1,
                                                &shown));
}

void test_bowl_grams_display_digits_caps_at_999(void)
{
    uint16_t shown = 0u;

    TEST_ASSERT_EQUAL(BOWL_DISPLAY_GRAMS,
                      bowl_grams_display_digits(WEIGHT_CAL_SUCCESS, true, 1500, &shown));
    TEST_ASSERT_EQUAL(999u, shown);
}

void test_bowl_grams_display_digits_blank_without_sample(void)
{
    TEST_ASSERT_EQUAL(BOWL_DISPLAY_BLANK,
                      bowl_grams_display_digits(WEIGHT_CAL_SUCCESS, false, 42, NULL));
}
