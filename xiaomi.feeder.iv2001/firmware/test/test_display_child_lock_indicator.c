/* Tests: spec/30-processes/display-presentation.md § Lock indicator */

#include "unity.h"

#include "display_child_lock_indicator.h"
#include "display_glyph.h"
#include "display_presentation.h"
#include "fake_display_port.h"
#include "tm1637.h"

static void child_lock_indicator_test_reset(void)
{
    fake_display_port_reset();
    display_presentation_reset();
    display_child_lock_indicator_test_reset();
}

void test_display_child_lock_indicator_blocked_blanks_digits_and_blinks(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    child_lock_indicator_test_reset();
    (void)display_presentation_set_digits(42u);
    (void)display_presentation_set_unit(DISPLAY_UNIT_GRAM);
    (void)display_presentation_icon_set(DISPLAY_ICON_CHILD_LOCK, true);

    display_child_lock_indicator_blocked_feedback(true, 0u);
    TEST_ASSERT_TRUE(display_child_lock_indicator_feedback_active());
    (void)display_presentation_tick(0u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_UINT8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_UINT8(0x00u, grids[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00u, grids[2]);
    TEST_ASSERT_NOT_EQUAL(0x00u, grids[3] | grids[4]);
}

void test_display_child_lock_indicator_poll_restores_after_one_second(void)
{
    child_lock_indicator_test_reset();

    display_child_lock_indicator_blocked_feedback(true, 0u);
    TEST_ASSERT_FALSE(display_child_lock_indicator_poll(999u));
    TEST_ASSERT_TRUE(display_child_lock_indicator_feedback_active());
    TEST_ASSERT_TRUE(display_child_lock_indicator_poll(1000u));
    TEST_ASSERT_FALSE(display_child_lock_indicator_feedback_active());
}
