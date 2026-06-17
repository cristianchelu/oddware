/*
 * OTA digit progress policy — spec/30-processes/display-presentation.md § OTA indicator
 */

#ifndef DISPLAY_OTA_INDICATOR_H
#define DISPLAY_OTA_INDICATOR_H

#include "ota_port.h"

void display_ota_indicator_start(void);
void display_ota_indicator_on_progress(const ota_progress_t *progress);

#endif /* DISPLAY_OTA_INDICATOR_H */
