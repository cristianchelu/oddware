/*
 * Dispensing pictograph policy — spec/30-processes/display-presentation.md
 */

#ifndef DISPLAY_DISPENSE_INDICATOR_H
#define DISPLAY_DISPENSE_INDICATOR_H

#include "port_err.h"

port_err_t display_dispense_indicator_active(void);
void display_dispense_indicator_idle(void);

#endif /* DISPLAY_DISPENSE_INDICATOR_H */
