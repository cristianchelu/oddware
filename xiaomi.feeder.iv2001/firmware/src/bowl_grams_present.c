/*
 * Bowl grams presentation — spec/30-processes/weighing.md, mqtt-protocol.md § Bowl weight
 */

#include "bowl_grams_present.h"

#include <stddef.h>

#include "weigh_product.h"

static bool bowl_grams_sample_plausible(int32_t food_g)
{
    return food_g <= BOWL_GRAMS_IMPLAUSIBLE_HIGH_G &&
           food_g >= BOWL_GRAMS_IMPLAUSIBLE_LOW_G;
}

static bool bowl_grams_bowl_present(int32_t food_g)
{
    return food_g >= -(int32_t)WEIGH_BOWL_MISSING_THRESHOLD_G;
}

bowl_grams_status_t bowl_grams_present(weight_cal_status_t cal,
                                       bool sample_valid,
                                       int32_t food_g,
                                       int32_t *out_g)
{
    if (out_g != NULL) {
        *out_g = 0;
    }

    if (cal != WEIGHT_CAL_SUCCESS || !sample_valid ||
        !bowl_grams_bowl_present(food_g) ||
        !bowl_grams_sample_plausible(food_g)) {
        return BOWL_GRAMS_UNKNOWN;
    }

    if (out_g != NULL) {
        if (food_g < 0) {
            *out_g = 0;
        } else {
            *out_g = food_g;
        }
    }

    return BOWL_GRAMS_KNOWN;
}

bowl_display_digits_t bowl_grams_display_digits(weight_cal_status_t cal,
                                              bool sample_valid,
                                              int32_t food_g,
                                              uint16_t *out_shown_g)
{
    if (out_shown_g != NULL) {
        *out_shown_g = 0u;
    }

    if (cal != WEIGHT_CAL_SUCCESS) {
        return BOWL_DISPLAY_DASH;
    }

    if (!sample_valid || !bowl_grams_sample_plausible(food_g)) {
        return BOWL_DISPLAY_BLANK;
    }

    if (!bowl_grams_bowl_present(food_g)) {
        return BOWL_DISPLAY_UNDERFLOW;
    }

    if (out_shown_g != NULL) {
        if (food_g < 0) {
            *out_shown_g = 0u;
        } else if (food_g > (int32_t)BOWL_GRAMS_DISPLAY_MAX_G) {
            *out_shown_g = BOWL_GRAMS_DISPLAY_MAX_G;
        } else {
            *out_shown_g = (uint16_t)food_g;
        }
    }

    return BOWL_DISPLAY_GRAMS;
}
