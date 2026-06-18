/*
 * Dispensing pictograph policy — spec/30-processes/display-presentation.md
 */

#include "display_dispense_indicator.h"

#include "app_log.h"
#include "display_glyph.h"
#include "display_presentation.h"

#define DISPLAY_DISPENSE_INDICATOR_ON_MS   150u
#define DISPLAY_DISPENSE_INDICATOR_OFF_MS  150u

port_err_t display_dispense_indicator_active(void)
{
    port_err_t err;

    (void)display_presentation_icon_set(DISPLAY_ICON_DISPENSING, false);
    err = display_presentation_icon_blink(DISPLAY_ICON_DISPENSING,
                                          DISPLAY_DISPENSE_INDICATOR_ON_MS,
                                          DISPLAY_DISPENSE_INDICATOR_OFF_MS);
    if (err != PORT_OK) {
        app_log_info("display", "dispense indicator blink err=%d", (int)err);
        return err;
    }

    (void)display_presentation_refresh();
    return PORT_OK;
}

void display_dispense_indicator_idle(void)
{
    (void)display_presentation_icon_blink_stop(DISPLAY_ICON_DISPENSING);
    (void)display_presentation_icon_set(DISPLAY_ICON_DISPENSING, false);
    (void)display_presentation_refresh();
}
