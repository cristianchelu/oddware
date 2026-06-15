/*
 * SDK STA NVDM profile — spec/30-processes/wifi-lifecycle.md
 */

#include "wifi_sdk_hal.h"
#include "wifi_sdk_profile.h"

void wifi_sdk_profile_invalidate(void)
{
    const char zero[] = "0";

    (void)wifi_sdk_hal_stop_scan();
    (void)wifi_sdk_hal_disconnect_ap();
    (void)wifi_sdk_hal_nvdm_write_string("STA",
                                         "SsidLen",
                                         (const uint8_t *)zero,
                                         1u);
    (void)wifi_sdk_hal_nvdm_write_string("STA",
                                         "Ssid",
                                         (const uint8_t *)"",
                                         0u);
    (void)wifi_sdk_hal_nvdm_write_string("STA",
                                         "WpaPskLen",
                                         (const uint8_t *)zero,
                                         1u);
    (void)wifi_sdk_hal_nvdm_write_string("STA",
                                         "WpaPsk",
                                         (const uint8_t *)"",
                                         0u);
    (void)wifi_sdk_hal_reload_setting();
}
