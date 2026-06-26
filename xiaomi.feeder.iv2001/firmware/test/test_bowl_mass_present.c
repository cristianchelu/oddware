/* Tests: spec/30-processes/weighing.md, mqtt-protocol.md § Bowl weight */

#include "unity.h"

#include "bowl_mass_present.h"
#include "weigh_product.h"
#include "weight_units.h"

void test_bowl_mass_present_uncalibrated_is_unknown(void)
{
    weight_dg_t g = -1;

    TEST_ASSERT_EQUAL(BOWL_MASS_UNKNOWN,
                      bowl_mass_present_dg(WEIGHT_CAL_UNCALIBRATED, true, 420, &g));
}

void test_bowl_mass_present_bowl_missing_is_unknown(void)
{
    weight_dg_t g = 0;

    TEST_ASSERT_EQUAL(BOWL_MASS_UNKNOWN,
                      bowl_mass_present_dg(WEIGHT_CAL_SUCCESS,
                                           true,
                                           -WEIGH_BOWL_MISSING_THRESHOLD_DG - 1,
                                           &g));
}

void test_bowl_mass_present_no_sample_is_unknown(void)
{
    weight_dg_t g = 0;

    TEST_ASSERT_EQUAL(BOWL_MASS_UNKNOWN,
                      bowl_mass_present_dg(WEIGHT_CAL_SUCCESS, false, 420, &g));
}

void test_bowl_mass_present_implausible_high_is_unknown(void)
{
    TEST_ASSERT_EQUAL(BOWL_MASS_UNKNOWN,
                      bowl_mass_present_dg(WEIGHT_CAL_SUCCESS,
                                           true,
                                           BOWL_MASS_IMPLAUSIBLE_HIGH_DG + 1,
                                           NULL));
}

void test_bowl_mass_present_negative_drift_is_zero(void)
{
    weight_dg_t g = 0;

    TEST_ASSERT_EQUAL(BOWL_MASS_KNOWN,
                      bowl_mass_present_dg(WEIGHT_CAL_SUCCESS, true, -10, &g));
    TEST_ASSERT_EQUAL_INT32(0, g);
}

void test_bowl_mass_present_known_positive(void)
{
    weight_dg_t g = 0;

    TEST_ASSERT_EQUAL(BOWL_MASS_KNOWN,
                      bowl_mass_present_dg(WEIGHT_CAL_SUCCESS, true, 423, &g));
    TEST_ASSERT_EQUAL_INT32(423, g);
}

void test_bowl_mass_present_full_range_not_capped(void)
{
    weight_dg_t g = 0;

    TEST_ASSERT_EQUAL(BOWL_MASS_KNOWN,
                      bowl_mass_present_dg(WEIGHT_CAL_SUCCESS, true, 15000, &g));
    TEST_ASSERT_EQUAL_INT32(15000, g);
}

void test_bowl_mass_display_digits_uncalibrated_dash(void)
{
    uint16_t shown = 99u;

    TEST_ASSERT_EQUAL(BOWL_DISPLAY_DASH,
                      bowl_mass_display_digits(WEIGHT_CAL_UNCALIBRATED, true, 420, &shown));
}

void test_bowl_mass_display_digits_underflow(void)
{
    uint16_t shown = 0u;

    TEST_ASSERT_EQUAL(BOWL_DISPLAY_UNDERFLOW,
                      bowl_mass_display_digits(WEIGHT_CAL_SUCCESS,
                                               true,
                                               -WEIGH_BOWL_MISSING_THRESHOLD_DG - 1,
                                               &shown));
}

void test_bowl_mass_display_digits_caps_at_999(void)
{
    uint16_t shown = 0u;

    TEST_ASSERT_EQUAL(BOWL_DISPLAY_GRAMS,
                      bowl_mass_display_digits(WEIGHT_CAL_SUCCESS, true, 15000, &shown));
    TEST_ASSERT_EQUAL_UINT16(999, shown);
}

void test_bowl_mass_display_digits_rounds_to_whole_grams(void)
{
    uint16_t shown = 0u;

    TEST_ASSERT_EQUAL(BOWL_DISPLAY_GRAMS,
                      bowl_mass_display_digits(WEIGHT_CAL_SUCCESS, true, 423, &shown));
    TEST_ASSERT_EQUAL_UINT16(42, shown);

    TEST_ASSERT_EQUAL(BOWL_DISPLAY_GRAMS,
                      bowl_mass_display_digits(WEIGHT_CAL_SUCCESS, true, 425, &shown));
    TEST_ASSERT_EQUAL_UINT16(43, shown);
}

void test_bowl_mass_display_digits_blank_without_sample(void)
{
    TEST_ASSERT_EQUAL(BOWL_DISPLAY_BLANK,
                      bowl_mass_display_digits(WEIGHT_CAL_SUCCESS, false, 420, NULL));
}
