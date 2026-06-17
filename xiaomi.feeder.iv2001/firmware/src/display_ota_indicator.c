/*
 * OTA digit progress policy — spec/30-processes/display-presentation.md § OTA indicator
 */

#include "display_ota_indicator.h"
#include "display_presentation.h"

void display_ota_indicator_start(void)
{
    (void)display_presentation_ota_show(DISPLAY_OTA_PHASE_CONNECTING, 0u);
}

void display_ota_indicator_on_progress(const ota_progress_t *progress)
{
    if (progress == NULL) {
        return;
    }

    switch (progress->status) {
    case OTA_STATUS_PREPARING:
    case OTA_STATUS_CONNECTING:
        (void)display_presentation_ota_show(DISPLAY_OTA_PHASE_CONNECTING, 0u);
        break;
    case OTA_STATUS_DOWNLOADING:
        (void)display_presentation_ota_show(DISPLAY_OTA_PHASE_DOWNLOADING,
                                            progress->pct);
        break;
    case OTA_STATUS_VERIFYING:
        (void)display_presentation_ota_show(DISPLAY_OTA_PHASE_VERIFYING,
                                            progress->pct);
        break;
    case OTA_STATUS_APPLYING:
        (void)display_presentation_ota_stop();
        break;
    case OTA_STATUS_ERROR:
    case OTA_STATUS_IDLE:
        (void)display_presentation_ota_stop();
        break;
    default:
        break;
    }
}
