/* Tests: spec/30-processes/display-presentation.md § MQTT indicator */

#include "unity.h"

#include "display_mqtt_indicator.h"
#include "display_presentation.h"
#include "fake_display_port.h"

static void mqtt_indicator_test_setup(void)
{
    fake_display_port_reset();
    display_presentation_reset();
}

void test_display_mqtt_indicator_connected_green_on(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    mqtt_indicator_test_setup();
    display_mqtt_indicator_connected();
    (void)display_presentation_tick(0u);

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[4]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4] & 0x01u);
}

void test_display_mqtt_indicator_off_clears_bars(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    mqtt_indicator_test_setup();
    display_mqtt_indicator_connected();
    display_mqtt_indicator_off();
    (void)display_presentation_tick(0u);

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);
}

void test_display_mqtt_indicator_connecting_inverted_blink(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    mqtt_indicator_test_setup();
    display_mqtt_indicator_connecting();

    TEST_ASSERT_EQUAL(1800u, display_presentation_tick(0u));

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x01u, grids[4]);

    (void)display_presentation_tick(1800u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);

    (void)display_presentation_tick(2000u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x01u, grids[4]);
}

void test_display_mqtt_indicator_error_pattern(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    mqtt_indicator_test_setup();
    display_mqtt_indicator_error();

    TEST_ASSERT_EQUAL(150u, display_presentation_tick(0u));

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);

    (void)display_presentation_tick(300u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x01u, grids[4]);

    (void)display_presentation_tick(900u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[4]);
}

void test_display_mqtt_indicator_connected_clears_orange_pattern(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    mqtt_indicator_test_setup();
    display_mqtt_indicator_error();
    display_mqtt_indicator_connected();
    (void)display_presentation_tick(0u);

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[4]);

    (void)display_presentation_tick(150u);
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[4]);
}
