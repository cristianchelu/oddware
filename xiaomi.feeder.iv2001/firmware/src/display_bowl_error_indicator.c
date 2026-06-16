/*
 * Food-bowl pictograph policy — spec/30-processes/display-presentation.md
 * § Bowl error indicator
 */

#include "display_bowl_error_indicator.h"

#include "display_glyph.h"
#include "display_presentation.h"

#define DISPLAY_BOWL_ERROR_CAL_ON_MS    600u
#define DISPLAY_BOWL_ERROR_CAL_OFF_MS   600u
#define DISPLAY_BOWL_ERROR_SPAN_ON_MS   200u
#define DISPLAY_BOWL_ERROR_SPAN_OFF_MS  200u

static void display_bowl_error_indicator_blink(uint16_t on_ms, uint16_t off_ms)
{
    (void)display_presentation_icon_set(DISPLAY_ICON_BOWL_ERROR, false);
    (void)display_presentation_icon_blink(DISPLAY_ICON_BOWL_ERROR, on_ms, off_ms);
}

void display_bowl_error_indicator_sync(bowl_error_kind_t kind)
{
    switch (kind) {
    case BOWL_ERROR_CAL_INCOMPLETE:
        display_bowl_error_indicator_blink(DISPLAY_BOWL_ERROR_CAL_ON_MS,
                                           DISPLAY_BOWL_ERROR_CAL_OFF_MS);
        break;

    case BOWL_ERROR_CAL_SPAN_PENDING:
        display_bowl_error_indicator_blink(DISPLAY_BOWL_ERROR_SPAN_ON_MS,
                                           DISPLAY_BOWL_ERROR_SPAN_OFF_MS);
        break;

    case BOWL_ERROR_BOWL_MISSING:
        (void)display_presentation_icon_blink_stop(DISPLAY_ICON_BOWL_ERROR);
        (void)display_presentation_icon_set(DISPLAY_ICON_BOWL_ERROR, true);
        break;

    case BOWL_ERROR_NONE:
    default:
        (void)display_presentation_icon_blink_stop(DISPLAY_ICON_BOWL_ERROR);
        (void)display_presentation_icon_set(DISPLAY_ICON_BOWL_ERROR, false);
        break;
    }
}
