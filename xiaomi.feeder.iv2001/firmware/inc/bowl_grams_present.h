/*
 * Bowl grams presentation — spec/30-processes/weighing.md, mqtt-protocol.md § Bowl weight
 */

#ifndef BOWL_GRAMS_PRESENT_H
#define BOWL_GRAMS_PRESENT_H

#include <stdbool.h>
#include <stdint.h>

#include "weight_port.h"

typedef enum {
    BOWL_GRAMS_UNKNOWN = 0,
    BOWL_GRAMS_KNOWN,
} bowl_grams_status_t;

typedef enum {
    BOWL_DISPLAY_DASH = 0,
    BOWL_DISPLAY_UNDERFLOW,
    BOWL_DISPLAY_BLANK,
    BOWL_DISPLAY_GRAMS,
} bowl_display_digits_t;

#define BOWL_GRAMS_IMPLAUSIBLE_HIGH_G  5000
#define BOWL_GRAMS_IMPLAUSIBLE_LOW_G   (-100)
#define BOWL_GRAMS_DISPLAY_MAX_G       999u

bowl_grams_status_t bowl_grams_present(weight_cal_status_t cal,
                                       bool sample_valid,
                                       int32_t food_g,
                                       int32_t *out_g);

bowl_display_digits_t bowl_grams_display_digits(weight_cal_status_t cal,
                                              bool sample_valid,
                                              int32_t food_g,
                                              uint16_t *out_shown_g);

#endif /* BOWL_GRAMS_PRESENT_H */
