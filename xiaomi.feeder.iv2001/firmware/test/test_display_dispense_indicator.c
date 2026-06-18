/* Tests: spec/30-processes/display-presentation.md § Dispensing indicator */

#include "unity.h"

#include "display_dispense_indicator.h"
#include "display_presentation.h"
#include "fake_display_port.h"
#include "port_err.h"
#include "tm1637.h"

static void dispense_indicator_test_setup(void)
{
    fake_display_port_reset();
    display_presentation_reset();
}

static bool dispense_grid_visible(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    fake_display_port_last_grids(grids);
    return (grids[3] & 0x04u) != 0u;
}

void test_display_dispense_indicator_blinks_while_active(void)
{
    uint32_t now_ms = 0u;
    bool saw_on = false;
    bool saw_off = false;

    dispense_indicator_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_dispense_indicator_active());

    (void)display_presentation_tick(now_ms);
    saw_on = dispense_grid_visible();

    for (uint32_t step = 1u; step <= 24u; step++) {
        now_ms += 50u;
        (void)display_presentation_tick(now_ms);
        if (dispense_grid_visible()) {
            saw_on = true;
        } else {
            saw_off = true;
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(saw_on, "dispensing icon should turn on");
    TEST_ASSERT_TRUE_MESSAGE(saw_off, "dispensing icon should turn off");
}

void test_display_dispense_indicator_hidden_when_ota_active(void)
{
    dispense_indicator_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_dispense_indicator_active());
    TEST_ASSERT_EQUAL(PORT_OK,
                      display_presentation_ota_show(DISPLAY_OTA_PHASE_DOWNLOADING, 0u));

    (void)display_presentation_tick(0u);
    (void)display_presentation_refresh();

    TEST_ASSERT_FALSE(dispense_grid_visible());
}

void test_display_dispense_indicator_fails_when_blink_slots_full(void)
{
    dispense_indicator_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK,
                      display_presentation_icon_blink(DISPLAY_ICON_WIFI, 200u, 800u));
    TEST_ASSERT_EQUAL(PORT_OK,
                      display_presentation_icon_blink(DISPLAY_ICON_BOWL_ERROR,
                                                      200u, 800u));
    TEST_ASSERT_EQUAL(PORT_OK,
                      display_presentation_icon_blink(DISPLAY_ICON_BAR_ORANGE,
                                                      200u, 800u));
    TEST_ASSERT_EQUAL(PORT_OK,
                      display_presentation_icon_blink(DISPLAY_ICON_CHILD_LOCK,
                                                      200u, 800u));

    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, display_dispense_indicator_active());

    (void)display_presentation_tick(0u);
    TEST_ASSERT_FALSE(dispense_grid_visible());
}

void test_display_dispense_indicator_blink_resumes_after_animation(void)
{
    dispense_indicator_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_dispense_indicator_active());

    (void)display_presentation_tick(0u);
    TEST_ASSERT_TRUE(dispense_grid_visible());

    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_play_builtin(DISPLAY_ANIM_OTA, true));
    (void)display_presentation_tick(400u);

    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_stop_animation());
    (void)display_presentation_refresh();
    TEST_ASSERT_TRUE_MESSAGE(dispense_grid_visible(),
                             "blink phase frozen during animation should resume on");

    (void)display_presentation_tick(600u);
    TEST_ASSERT_FALSE_MESSAGE(dispense_grid_visible(),
                              "blink should advance after animation ends");
}
