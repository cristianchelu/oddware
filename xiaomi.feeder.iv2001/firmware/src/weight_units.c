/*
 * Weight unit helpers — spec/30-processes/weighing.md § Data model
 */

#include <stdio.h>

#include "weight_units.h"

uint16_t weight_dg_to_display_g(weight_dg_t dg)
{
    int32_t rounded = WEIGHT_DG_TO_G_ROUND(dg);

    if (rounded < 0) {
        return 0u;
    }

    if (rounded > (int32_t)WEIGHT_DISPLAY_MAX_G) {
        return WEIGHT_DISPLAY_MAX_G;
    }

    return (uint16_t)rounded;
}

int weight_format_mqtt_g(weight_dg_t dg, char *buf, size_t len)
{
    int32_t clamped = dg;

    /* MQTT bowl_weight clamps small negatives to 0.0 (spec mqtt-protocol.md). */
    if (clamped < 0) {
        clamped = 0;
    }

    return snprintf(buf,
                    len,
                    "%ld.%ld",
                    (long)(clamped / 10),
                    (long)(clamped % 10));
}

/*
 * UART weigh/tare diagnostics keep signed values (e.g. bowl removed).
 * Unlike weight_format_mqtt_g(), negatives are not clamped.
 */
int weight_format_cli_g(weight_dg_t dg, char *buf, size_t len)
{
    int32_t whole = dg / 10;
    int32_t frac = dg % 10;
    int32_t abs_frac = frac < 0 ? -frac : frac;

    if (frac < 0 && whole == 0) {
        return snprintf(buf, len, "-0.%ld", (long)abs_frac);
    }

    return snprintf(buf, len, "%ld.%ld", (long)whole, (long)abs_frac);
}
