/* Tests: spec/30-processes/battery-monitoring.md § Chemistry and discharge curves */

#include "unity.h"

#include "battery_pct.h"

void test_battery_pct_default_chemistry_is_aa_alk_4s(void)
{
    TEST_ASSERT_EQUAL(BATTERY_CHEM_AA_ALK_4S, battery_pct_default_chemistry());
}

void test_battery_pct_clamps_above_full(void)
{
    TEST_ASSERT_EQUAL_UINT8(100u, battery_pct_from_mv(6500u, BATTERY_CHEM_AA_ALK_4S));
}

void test_battery_pct_clamps_below_empty(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, battery_pct_from_mv(3900u, BATTERY_CHEM_AA_ALK_4S));
}

void test_battery_pct_at_knots(void)
{
    TEST_ASSERT_EQUAL_UINT8(100u, battery_pct_from_mv(6000u, BATTERY_CHEM_AA_ALK_4S));
    TEST_ASSERT_EQUAL_UINT8(75u, battery_pct_from_mv(5600u, BATTERY_CHEM_AA_ALK_4S));
    TEST_ASSERT_EQUAL_UINT8(50u, battery_pct_from_mv(5200u, BATTERY_CHEM_AA_ALK_4S));
    TEST_ASSERT_EQUAL_UINT8(25u, battery_pct_from_mv(4800u, BATTERY_CHEM_AA_ALK_4S));
    TEST_ASSERT_EQUAL_UINT8(10u, battery_pct_from_mv(4400u, BATTERY_CHEM_AA_ALK_4S));
    TEST_ASSERT_EQUAL_UINT8(0u, battery_pct_from_mv(4000u, BATTERY_CHEM_AA_ALK_4S));
}

void test_battery_pct_interpolates_mid_segment(void)
{
    TEST_ASSERT_EQUAL_UINT8(88u, battery_pct_from_mv(5800u, BATTERY_CHEM_AA_ALK_4S));
    TEST_ASSERT_EQUAL_UINT8(63u, battery_pct_from_mv(5400u, BATTERY_CHEM_AA_ALK_4S));
}

void test_battery_pct_unknown_chemistry_falls_back_to_default(void)
{
    TEST_ASSERT_EQUAL_UINT8(50u,
                            battery_pct_from_mv(5200u,
                                                (battery_chemistry_t)BATTERY_CHEM_COUNT));
}
