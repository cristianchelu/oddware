/*
 * Bowl mass presentation — spec/30-processes/weighing.md, mqtt-protocol.md § Bowl weight
 */

#ifndef BOWL_MASS_PRESENT_H
#define BOWL_MASS_PRESENT_H

#include <stdbool.h>
#include <stdint.h>

#include "weight_port.h"
#include "weight_units.h"

typedef enum {
    BOWL_MASS_UNKNOWN = 0,
    BOWL_MASS_KNOWN,
} bowl_mass_status_t;

typedef enum {
    BOWL_DISPLAY_DASH = 0,
    BOWL_DISPLAY_UNDERFLOW,
    BOWL_DISPLAY_BLANK,
    BOWL_DISPLAY_GRAMS,
} bowl_display_digits_t;

#define BOWL_MASS_IMPLAUSIBLE_HIGH_DG  WEIGHT_G_TO_DG(5000)
#define BOWL_MASS_IMPLAUSIBLE_LOW_DG   WEIGHT_G_TO_DG(-100)

bowl_mass_status_t bowl_mass_present_dg(weight_cal_status_t cal,
                                        bool sample_valid,
                                        weight_dg_t food_dg,
                                        weight_dg_t *out_dg);

bowl_display_digits_t bowl_mass_display_digits(weight_cal_status_t cal,
                                               bool sample_valid,
                                               weight_dg_t food_dg,
                                               uint16_t *out_shown_g);

#endif /* BOWL_MASS_PRESENT_H */
