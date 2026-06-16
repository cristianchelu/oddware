/*
 * Bowl error evaluation — spec/30-processes/weighing.md § Bowl presence
 */

#ifndef BOWL_ERROR_H
#define BOWL_ERROR_H

#include <stdbool.h>
#include <stdint.h>

#include "weight_port.h"

typedef enum {
    BOWL_ERROR_NONE = 0,
    BOWL_ERROR_CAL_INCOMPLETE,
    BOWL_ERROR_CAL_SPAN_PENDING,
    BOWL_ERROR_BOWL_MISSING,
} bowl_error_kind_t;

bowl_error_kind_t bowl_error_eval(weight_cal_status_t cal,
                                  bool sample_valid,
                                  int32_t food_g);
bool bowl_error_is_active(bowl_error_kind_t kind);

#endif /* BOWL_ERROR_H */
