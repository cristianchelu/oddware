/*
 * SDK Wi-Fi HAL shims — spec/30-processes/wifi-lifecycle.md (SDK STA profile invalidate)
 *
 * Adapter implements on device; host test shim records NVDM writes.
 */

#ifndef WIFI_SDK_HAL_H
#define WIFI_SDK_HAL_H

#include <stdint.h>

#include "port_err.h"

port_err_t wifi_sdk_hal_stop_scan(void);
port_err_t wifi_sdk_hal_disconnect_ap(void);
port_err_t wifi_sdk_hal_reload_setting(void);
port_err_t wifi_sdk_hal_nvdm_write_string(const char *group,
                                          const char *key,
                                          const uint8_t *data,
                                          uint32_t len);

#endif /* WIFI_SDK_HAL_H */
