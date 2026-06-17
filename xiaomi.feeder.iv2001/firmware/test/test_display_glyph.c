/* Tests: spec/30-processes/display-presentation.md § Logical API */

#include "unity.h"

#include "display_glyph.h"

void test_display_glyph_digit_lut_matches_hardware_spec(void)
{
    static const uint8_t expected[] = {
        0x3Fu, 0x06u, 0x5Bu, 0x4Fu, 0x66u, 0x6Du, 0x7Du, 0x07u, 0x7Fu, 0x6Fu,
    };

    for (uint8_t d = 0u; d < 10u; d++) {
        TEST_ASSERT_EQUAL_HEX8(expected[d], display_glyph_digit_segment(d));
    }
}

void test_display_compose_grids_leading_zero_suppression(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_compose_grids(true, 42u, DISPLAY_UNIT_NONE, 0u, grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x66u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x5Bu, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[3]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);
}

void test_display_compose_grids_single_digit_zero(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_compose_grids(true, 0u, DISPLAY_UNIT_NONE, 0u, grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x3Fu, grids[2]);
}

void test_display_compose_grids_unit_gram(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_compose_grids(true, 42u, DISPLAY_UNIT_GRAM, 0u, grids);
    TEST_ASSERT_EQUAL_HEX8(0x10u, grids[3]);
}

void test_display_compose_grids_unit_percent(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_compose_grids(true, 100u, DISPLAY_UNIT_PERCENT, 0u, grids);
    TEST_ASSERT_EQUAL_HEX8(0x08u, grids[3]);
}

void test_display_compose_grids_wifi_icon(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_compose_grids(true, 0u, DISPLAY_UNIT_NONE,
                          DISPLAY_GLYPH_ICON_MASK(DISPLAY_ICON_WIFI), grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);
}

void test_display_compose_grids_bowl_error_on_grid4(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_compose_grids(true, 0u, DISPLAY_UNIT_NONE,
                          DISPLAY_GLYPH_ICON_MASK(DISPLAY_ICON_BOWL_ERROR), grids);
    TEST_ASSERT_EQUAL_HEX8(0x04u, grids[4]);
}

void test_display_compose_grids_hidden_digits_blank_grids(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_compose_grids(false, 0u, DISPLAY_UNIT_NONE,
                          DISPLAY_GLYPH_ICON_MASK(DISPLAY_ICON_WIFI), grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);
}

void test_display_compose_grids_combined_scene(void)
{
    uint8_t grids[TM1637_GRID_COUNT];
    uint32_t icons = DISPLAY_GLYPH_ICON_MASK(DISPLAY_ICON_WIFI) |
                     DISPLAY_GLYPH_ICON_MASK(DISPLAY_ICON_DISPENSING) |
                     DISPLAY_GLYPH_ICON_MASK(DISPLAY_ICON_BAR_GREEN);

    display_compose_grids(true, 7u, DISPLAY_UNIT_GRAM, icons, grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x07u, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x16u, grids[3]); /* wifi + dispensing + gram */
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[4]);
}

void test_display_glyph_ota_bar_g_only(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_glyph_ota_bar(0u, true, grids);
    TEST_ASSERT_EQUAL_HEX8(0x40u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x40u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x40u, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[3]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);
}

void test_display_glyph_ota_bar_five_segments(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_glyph_ota_bar(5u, true, grids);
    TEST_ASSERT_EQUAL_HEX8(0x41u, grids[0]); /* A + G */
    TEST_ASSERT_EQUAL_HEX8(0x41u, grids[1]); /* A + G */
    TEST_ASSERT_EQUAL_HEX8(0x47u, grids[2]); /* A+B+C + G */
}

void test_display_glyph_ota_bar_full_perimeter(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    display_glyph_ota_bar(10u, true, grids);
    TEST_ASSERT_EQUAL_HEX8(0x79u, grids[0]); /* A+D+E+F + G */
    TEST_ASSERT_EQUAL_HEX8(0x49u, grids[1]); /* A+D + G */
    TEST_ASSERT_EQUAL_HEX8(0x4Fu, grids[2]); /* A+B+C+D + G */
}

void test_display_glyph_ota_filled_from_pct_ceil(void)
{
    TEST_ASSERT_EQUAL_UINT8(0u, display_glyph_ota_filled_from_pct(0u));
    TEST_ASSERT_EQUAL_UINT8(1u, display_glyph_ota_filled_from_pct(1u));
    TEST_ASSERT_EQUAL_UINT8(5u, display_glyph_ota_filled_from_pct(45u));
    TEST_ASSERT_EQUAL_UINT8(10u, display_glyph_ota_filled_from_pct(100u));
}
