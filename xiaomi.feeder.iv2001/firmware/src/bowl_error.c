/*
 * Bowl error evaluation — spec/30-processes/weighing.md § Bowl presence
 */

#include "bowl_error.h"

#include "weigh_product.h"

bowl_error_kind_t bowl_error_eval(weight_cal_status_t cal,
                                  bool sample_valid,
                                  weight_dg_t food_dg)
{
    if (cal == WEIGHT_CAL_UNCALIBRATED || cal == WEIGHT_CAL_IDLE) {
        return BOWL_ERROR_CAL_INCOMPLETE;
    }

    if (cal == WEIGHT_CAL_CAPTURING_SPAN) {
        return BOWL_ERROR_CAL_SPAN_PENDING;
    }

    if (cal == WEIGHT_CAL_SUCCESS && sample_valid &&
        food_dg < -WEIGH_BOWL_MISSING_THRESHOLD_DG) {
        return BOWL_ERROR_BOWL_MISSING;
    }

    return BOWL_ERROR_NONE;
}

bool bowl_error_is_active(bowl_error_kind_t kind)
{
    return kind != BOWL_ERROR_NONE;
}
