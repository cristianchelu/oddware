/* Tests: spec/40-architecture/ports.md — weight_port read staging */

#include "unity.h"

#include "weight_port_read_staging.h"

/*
 * Regression: read_raw_grams must not clobber a prior read_dg slot
 * (weight_adapter.c).
 */
void test_weight_port_read_staging_dg_survives_raw_update(void)
{
    weight_port_read_staging_t staging = {
        .dg = 423,
        .raw_grams = 1500,
    };

    staging.raw_grams = 999;
    TEST_ASSERT_EQUAL_INT32(423, staging.dg);
    TEST_ASSERT_EQUAL_INT32(999, staging.raw_grams);
}

void test_weight_port_read_staging_raw_survives_dg_update(void)
{
    weight_port_read_staging_t staging = {
        .dg = 423,
        .raw_grams = 1500,
    };

    staging.dg = 100;
    TEST_ASSERT_EQUAL_INT32(100, staging.dg);
    TEST_ASSERT_EQUAL_INT32(1500, staging.raw_grams);
}
