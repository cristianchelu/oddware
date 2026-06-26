/* Tests: spec/30-processes/weighing.md § Bowl presence */

#include "unity.h"

#include "bowl_error.h"
#include "weigh_product.h"
#include "weight_units.h"

void test_bowl_error_eval_uncalibrated(void)
{
    TEST_ASSERT_EQUAL(BOWL_ERROR_CAL_INCOMPLETE,
                      bowl_error_eval(WEIGHT_CAL_UNCALIBRATED, false, 0));
    TEST_ASSERT_EQUAL(BOWL_ERROR_CAL_INCOMPLETE,
                      bowl_error_eval(WEIGHT_CAL_IDLE, true, 420));
}

void test_bowl_error_eval_span_pending(void)
{
    TEST_ASSERT_EQUAL(BOWL_ERROR_CAL_SPAN_PENDING,
                      bowl_error_eval(WEIGHT_CAL_CAPTURING_SPAN, false, 0));
}

void test_bowl_error_eval_calibrated_ok(void)
{
    TEST_ASSERT_EQUAL(BOWL_ERROR_NONE,
                      bowl_error_eval(WEIGHT_CAL_SUCCESS, true, 0));
    TEST_ASSERT_EQUAL(BOWL_ERROR_NONE,
                      bowl_error_eval(WEIGHT_CAL_SUCCESS, true, -10));
    TEST_ASSERT_EQUAL(BOWL_ERROR_NONE,
                      bowl_error_eval(WEIGHT_CAL_SUCCESS, false, -2000));
}

void test_bowl_error_eval_bowl_missing(void)
{
    TEST_ASSERT_EQUAL(BOWL_ERROR_BOWL_MISSING,
                      bowl_error_eval(WEIGHT_CAL_SUCCESS, true,
                                      -WEIGH_BOWL_MISSING_THRESHOLD_DG - 1));
}

void test_bowl_error_is_active(void)
{
    TEST_ASSERT_FALSE(bowl_error_is_active(BOWL_ERROR_NONE));
    TEST_ASSERT_TRUE(bowl_error_is_active(BOWL_ERROR_CAL_INCOMPLETE));
    TEST_ASSERT_TRUE(bowl_error_is_active(BOWL_ERROR_BOWL_MISSING));
}
