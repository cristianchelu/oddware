/* Tests: spec/30-processes/display-presentation.md */

#include <string.h>

#include "unity.h"

#include "display_presentation.h"
#include "fake_display_port.h"

static void presentation_test_setup(void)
{
    fake_display_port_reset();
    display_presentation_reset();
}

static size_t presentation_count_ops(fake_display_op_kind_t kind)
{
    size_t total;
    size_t n = 0u;
    const fake_display_op_t *ops = fake_display_port_ops(&total);

    for (size_t i = 0u; i < total; i++) {
        if (ops[i].kind == kind) {
            n++;
        }
    }
    return n;
}

void test_display_presentation_set_digits_rejects_overflow(void)
{
    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, display_presentation_set_digits(1000u));
}

void test_display_presentation_set_digits_underflow_single_dash(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_set_unit(DISPLAY_UNIT_GRAM));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_set_digits_underflow());
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x40u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x10u, grids[3]);
}

void test_display_presentation_refresh_repowers_after_expander_reset(void)
{
    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_set_digits(15u));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());
    TEST_ASSERT_EQUAL(1u, presentation_count_ops(FAKE_DISPLAY_OP_POWER_ON));
    TEST_ASSERT_EQUAL(1u, presentation_count_ops(FAKE_DISPLAY_OP_SHOW_GRIDS));

    display_presentation_note_expander_reset();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());
    TEST_ASSERT_EQUAL(2u, presentation_count_ops(FAKE_DISPLAY_OP_POWER_ON));
    TEST_ASSERT_EQUAL(2u, presentation_count_ops(FAKE_DISPLAY_OP_SHOW_GRIDS));
}

void test_display_presentation_refresh_shows_number_with_unit(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_set_digits(42u));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_set_unit(DISPLAY_UNIT_GRAM));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x66u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x5Bu, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x10u, grids[3]);
}

void test_display_presentation_icon_only_leaves_digits_blank(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_icon_set(DISPLAY_ICON_WIFI, true));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);
}

void test_display_presentation_icon_set_and_refresh(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_icon_set(DISPLAY_ICON_WIFI, true));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);
}

void test_display_presentation_wifi_blink_toggles_icon(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK,
                      display_presentation_icon_blink(DISPLAY_ICON_WIFI, 200u, 800u));
    TEST_ASSERT_EQUAL(200u, display_presentation_tick(0u));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);

    (void)display_presentation_tick(200u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[3]);

    (void)display_presentation_tick(1000u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);
}

void test_display_presentation_animation_ota_advances_frames(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_play_builtin(DISPLAY_ANIM_OTA, false));
    TEST_ASSERT_EQUAL(150u, display_presentation_tick(0u));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x01u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x04u, grids[1]);

    (void)display_presentation_tick(150u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x08u, grids[1]);
}

void test_display_presentation_stop_animation_restores_scene(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_set_digits(7u));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_play_builtin(DISPLAY_ANIM_OTA, true));
    (void)display_presentation_tick(0u);
    (void)display_presentation_refresh();

    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_stop_animation());
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x07u, grids[2]);
}

void test_display_presentation_clear_digits_blanks_readout(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_set_digits(42u));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_clear_digits());
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[2]);
}

void test_display_presentation_set_brightness(void)
{
    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_set_brightness(2u));
    TEST_ASSERT_EQUAL(2u, fake_display_port_brightness());
}

void test_display_presentation_parse_icon_names(void)
{
    display_icon_t icon;

    TEST_ASSERT_TRUE(display_presentation_parse_icon("wifi", &icon));
    TEST_ASSERT_EQUAL(DISPLAY_ICON_WIFI, icon);
    TEST_ASSERT_FALSE(display_presentation_parse_icon("unknown", &icon));
}

void test_display_presentation_icon_pattern_advances_phases(void)
{
    static const display_pattern_phase_t phases[] = {
        { 100u, false },
        { 100u, false },
        { 200u, true },
    };
    uint8_t grids[TM1637_GRID_COUNT];

    presentation_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK,
                      display_presentation_icon_pattern(DISPLAY_ICON_BAR_ORANGE,
                                                      phases,
                                                      3u,
                                                      true));
    TEST_ASSERT_EQUAL(100u, display_presentation_tick(0u));
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);

    (void)display_presentation_tick(200u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x01u, grids[4]);

    (void)display_presentation_tick(400u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);
}
