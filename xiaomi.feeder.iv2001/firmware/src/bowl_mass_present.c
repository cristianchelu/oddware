/*
 * Bowl mass presentation — spec/30-processes/weighing.md, mqtt-protocol.md § Bowl weight
 */

#include "bowl_mass_present.h"

#include <stddef.h>

#include "weigh_product.h"
#include "weight_units.h"

static bool bowl_mass_sample_plausible(weight_dg_t food_dg)
{
    return food_dg <= BOWL_MASS_IMPLAUSIBLE_HIGH_DG &&
           food_dg >= BOWL_MASS_IMPLAUSIBLE_LOW_DG;
}

static bool bowl_mass_bowl_present(weight_dg_t food_dg)
{
    return food_dg >= -WEIGH_BOWL_MISSING_THRESHOLD_DG;
}

bowl_mass_status_t bowl_mass_present_dg(weight_cal_status_t cal,
                                         bool sample_valid,
                                         weight_dg_t food_dg,
                                         weight_dg_t *out_dg)
{
    if (out_dg != NULL) {
        *out_dg = 0;
    }

    if (cal != WEIGHT_CAL_SUCCESS || !sample_valid ||
        !bowl_mass_bowl_present(food_dg) ||
        !bowl_mass_sample_plausible(food_dg)) {
        return BOWL_MASS_UNKNOWN;
    }

    if (out_dg != NULL) {
        if (food_dg < 0) {
            *out_dg = 0;
        } else {
            *out_dg = food_dg;
        }
    }

    return BOWL_MASS_KNOWN;
}

bowl_display_digits_t bowl_mass_display_digits(weight_cal_status_t cal,
                                               bool sample_valid,
                                               weight_dg_t food_dg,
                                               uint16_t *out_shown_g)
{
    if (out_shown_g != NULL) {
        *out_shown_g = 0u;
    }

    if (cal != WEIGHT_CAL_SUCCESS) {
        return BOWL_DISPLAY_DASH;
    }

    if (!sample_valid || !bowl_mass_sample_plausible(food_dg)) {
        return BOWL_DISPLAY_BLANK;
    }

    if (!bowl_mass_bowl_present(food_dg)) {
        return BOWL_DISPLAY_UNDERFLOW;
    }

    if (out_shown_g != NULL) {
        *out_shown_g = weight_dg_to_display_g(food_dg);
    }

    return BOWL_DISPLAY_GRAMS;
}
