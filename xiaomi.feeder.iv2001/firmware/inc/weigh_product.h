/*
 * Weigh product constants — spec/10-hardware/components/weigh-assp-cs1270.md
 */

#ifndef WEIGH_PRODUCT_H
#define WEIGH_PRODUCT_H

#include "weight_units.h"

/* Provided stainless bowl mass [product] */
#define WEIGH_BOWL_MASS_G  350
#define WEIGH_BOWL_MASS_DG WEIGHT_G_TO_DG(WEIGH_BOWL_MASS_G)
#define WEIGH_BOWL_MISSING_THRESHOLD_G  ((WEIGH_BOWL_MASS_G * 25) / 100)
#define WEIGH_BOWL_MISSING_THRESHOLD_DG WEIGHT_G_TO_DG(WEIGH_BOWL_MISSING_THRESHOLD_G)

#endif /* WEIGH_PRODUCT_H */
