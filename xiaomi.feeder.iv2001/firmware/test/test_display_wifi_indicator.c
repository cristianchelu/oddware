/* Tests: spec/30-processes/display-presentation.md § Wi-Fi indicator */

#include "unity.h"

#include "display_presentation.h"
#include "display_wifi_indicator.h"
#include "fake_display_port.h"

static void wifi_indicator_test_setup(void)
{
    fake_display_port_reset();
    display_presentation_reset();
}

void test_display_wifi_indicator_connected_steady_on(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    wifi_indicator_test_setup();
    display_wifi_indicator_connected();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);
}

void test_display_wifi_indicator_off_steady_off(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    wifi_indicator_test_setup();
    display_wifi_indicator_off();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[3]);
}

void test_display_wifi_indicator_connecting_blinks_500_500(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    wifi_indicator_test_setup();
    display_wifi_indicator_connecting();

    TEST_ASSERT_EQUAL(500u, display_presentation_tick(0u));

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);

    (void)display_presentation_tick(500u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[3]);

    (void)display_presentation_tick(1000u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);
}

void test_display_wifi_indicator_ap_mode_blinks_150_150(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    wifi_indicator_test_setup();
    display_wifi_indicator_ap_mode();

    TEST_ASSERT_EQUAL(150u, display_presentation_tick(0u));

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);

    (void)display_presentation_tick(150u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[3]);

    (void)display_presentation_tick(300u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);
}

void test_display_wifi_indicator_connected_stops_prior_blink(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    wifi_indicator_test_setup();
    display_wifi_indicator_connecting();
    display_wifi_indicator_connected();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);

    (void)display_presentation_tick(500u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3]);
}
