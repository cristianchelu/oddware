/*
 * MQTT status lightbar policy — spec/30-processes/display-presentation.md § MQTT indicator
 */

#include "display_glyph.h"
#include "display_mqtt_indicator.h"
#include "display_presentation.h"

#define DISPLAY_MQTT_INDICATOR_CONNECTING_ON_MS   1800u
#define DISPLAY_MQTT_INDICATOR_CONNECTING_OFF_MS   200u

#define DISPLAY_MQTT_INDICATOR_ERROR_OFF_MS          150u
#define DISPLAY_MQTT_INDICATOR_ERROR_ON_MS           600u

static const display_pattern_phase_t s_mqtt_error_pattern[] = {
    { DISPLAY_MQTT_INDICATOR_ERROR_OFF_MS, false },
    { DISPLAY_MQTT_INDICATOR_ERROR_OFF_MS, false },
    { DISPLAY_MQTT_INDICATOR_ERROR_ON_MS, true },
};

static void display_mqtt_indicator_clear_bars(void)
{
    (void)display_presentation_icon_pattern_stop(DISPLAY_ICON_BAR_ORANGE);
    (void)display_presentation_icon_blink_stop(DISPLAY_ICON_BAR_ORANGE);
    (void)display_presentation_icon_pattern_stop(DISPLAY_ICON_BAR_GREEN);
    (void)display_presentation_icon_blink_stop(DISPLAY_ICON_BAR_GREEN);
    (void)display_presentation_icon_set(DISPLAY_ICON_BAR_ORANGE, false);
    (void)display_presentation_icon_set(DISPLAY_ICON_BAR_GREEN, false);
}

void display_mqtt_indicator_connecting(void)
{
    display_mqtt_indicator_clear_bars();
    (void)display_presentation_icon_blink(DISPLAY_ICON_BAR_ORANGE,
                                          DISPLAY_MQTT_INDICATOR_CONNECTING_ON_MS,
                                          DISPLAY_MQTT_INDICATOR_CONNECTING_OFF_MS);
}

void display_mqtt_indicator_connected(void)
{
    display_mqtt_indicator_clear_bars();
    (void)display_presentation_icon_set(DISPLAY_ICON_BAR_GREEN, true);
}

void display_mqtt_indicator_error(void)
{
    display_mqtt_indicator_clear_bars();
    (void)display_presentation_icon_pattern(
        DISPLAY_ICON_BAR_ORANGE,
        s_mqtt_error_pattern,
        (uint8_t)(sizeof(s_mqtt_error_pattern) / sizeof(s_mqtt_error_pattern[0])),
        true);
}

void display_mqtt_indicator_off(void)
{
    display_mqtt_indicator_clear_bars();
}
