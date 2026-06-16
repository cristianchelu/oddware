/* Tests: spec/30-processes/display-presentation.md § Bowl error indicator */

#include "unity.h"

#include "bowl_error.h"
#include "display_bowl_error_indicator.h"
#include "display_presentation.h"
#include "fake_display_port.h"

static void bowl_indicator_test_setup(void)
{
    fake_display_port_reset();
    display_presentation_reset();
}

void test_display_bowl_error_indicator_uncalibrated_blinks_600_600(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    bowl_indicator_test_setup();
    display_bowl_error_indicator_sync(BOWL_ERROR_CAL_INCOMPLETE);

    TEST_ASSERT_EQUAL(600u, display_presentation_tick(0u));

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x04u, grids[4]);

    (void)display_presentation_tick(600u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);

    (void)display_presentation_tick(1200u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x04u, grids[4]);
}

void test_display_bowl_error_indicator_span_pending_blinks_200_200(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    bowl_indicator_test_setup();
    display_bowl_error_indicator_sync(BOWL_ERROR_CAL_SPAN_PENDING);

    TEST_ASSERT_EQUAL(200u, display_presentation_tick(0u));

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x04u, grids[4]);

    (void)display_presentation_tick(200u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);

    (void)display_presentation_tick(400u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x04u, grids[4]);
}

void test_display_bowl_error_indicator_calibrated_off(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    bowl_indicator_test_setup();
    display_bowl_error_indicator_sync(BOWL_ERROR_NONE);
    (void)display_presentation_tick(0u);

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);

    (void)display_presentation_tick(600u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);
}

void test_display_bowl_error_indicator_bowl_missing_steady_on(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    bowl_indicator_test_setup();
    display_bowl_error_indicator_sync(BOWL_ERROR_BOWL_MISSING);
    (void)display_presentation_tick(0u);

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x04u, grids[4]);

    (void)display_presentation_tick(600u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x04u, grids[4]);
}

void test_display_bowl_error_indicator_stops_blink_when_cal_complete(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    bowl_indicator_test_setup();
    display_bowl_error_indicator_sync(BOWL_ERROR_CAL_INCOMPLETE);
    display_bowl_error_indicator_sync(BOWL_ERROR_NONE);
    (void)display_presentation_tick(0u);

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);

    (void)display_presentation_tick(600u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);
}
