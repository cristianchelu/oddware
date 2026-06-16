/*
 * Food-bowl pictograph policy — spec/30-processes/display-presentation.md
 * § Bowl error indicator
 */

#ifndef DISPLAY_BOWL_ERROR_INDICATOR_H
#define DISPLAY_BOWL_ERROR_INDICATOR_H

#include "bowl_error.h"

void display_bowl_error_indicator_sync(bowl_error_kind_t kind);

#endif /* DISPLAY_BOWL_ERROR_INDICATOR_H */
