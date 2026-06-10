/*
 * Wi-Fi pictograph policy — spec/30-processes/display-presentation.md § Wi-Fi indicator
 */

#include "display_glyph.h"
#include "display_presentation.h"
#include "display_wifi_indicator.h"

#define DISPLAY_WIFI_INDICATOR_CONNECTING_ON_MS  500u
#define DISPLAY_WIFI_INDICATOR_CONNECTING_OFF_MS 500u
#define DISPLAY_WIFI_INDICATOR_AP_ON_MS            150u
#define DISPLAY_WIFI_INDICATOR_AP_OFF_MS           150u

static void display_wifi_indicator_blink(uint16_t on_ms, uint16_t off_ms)
{
    (void)display_presentation_icon_set(DISPLAY_ICON_WIFI, false);
    (void)display_presentation_icon_blink(DISPLAY_ICON_WIFI, on_ms, off_ms);
}

void display_wifi_indicator_connecting(void)
{
    display_wifi_indicator_blink(DISPLAY_WIFI_INDICATOR_CONNECTING_ON_MS,
                                 DISPLAY_WIFI_INDICATOR_CONNECTING_OFF_MS);
}

void display_wifi_indicator_ap_mode(void)
{
    display_wifi_indicator_blink(DISPLAY_WIFI_INDICATOR_AP_ON_MS,
                                 DISPLAY_WIFI_INDICATOR_AP_OFF_MS);
}

void display_wifi_indicator_connected(void)
{
    (void)display_presentation_icon_blink_stop(DISPLAY_ICON_WIFI);
    (void)display_presentation_icon_set(DISPLAY_ICON_WIFI, true);
}

void display_wifi_indicator_off(void)
{
    (void)display_presentation_icon_blink_stop(DISPLAY_ICON_WIFI);
    (void)display_presentation_icon_set(DISPLAY_ICON_WIFI, false);
}
