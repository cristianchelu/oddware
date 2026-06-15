/*
 * Dispensing pictograph policy — spec/30-processes/display-presentation.md
 */

#include "display_dispense_indicator.h"
#include "display_glyph.h"
#include "display_presentation.h"

#define DISPLAY_DISPENSE_INDICATOR_ON_MS   500u
#define DISPLAY_DISPENSE_INDICATOR_OFF_MS  500u

void display_dispense_indicator_active(void)
{
    (void)display_presentation_icon_set(DISPLAY_ICON_DISPENSING, false);
    (void)display_presentation_icon_blink(DISPLAY_ICON_DISPENSING,
                                         DISPLAY_DISPENSE_INDICATOR_ON_MS,
                                         DISPLAY_DISPENSE_INDICATOR_OFF_MS);
}

void display_dispense_indicator_idle(void)
{
    (void)display_presentation_icon_blink_stop(DISPLAY_ICON_DISPENSING);
    (void)display_presentation_icon_set(DISPLAY_ICON_DISPENSING, false);
}
