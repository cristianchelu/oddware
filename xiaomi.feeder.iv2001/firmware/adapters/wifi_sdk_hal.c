/*
 * SDK Wi-Fi HAL shims — spec/30-processes/wifi-lifecycle.md
 */

#include "nvdm.h"
#include "wifi_api.h"

#include "wifi_sdk_hal.h"

port_err_t wifi_sdk_hal_stop_scan(void)
{
    if (wifi_connection_stop_scan() < 0) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

port_err_t wifi_sdk_hal_disconnect_ap(void)
{
    if (wifi_connection_disconnect_ap() < 0) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

port_err_t wifi_sdk_hal_reload_setting(void)
{
    if (wifi_config_reload_setting() < 0) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

port_err_t wifi_sdk_hal_nvdm_write_string(const char *group,
                                          const char *key,
                                          const uint8_t *data,
                                          uint32_t len)
{
    if (group == NULL || key == NULL || data == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (nvdm_write_data_item(group,
                             key,
                             NVDM_DATA_ITEM_TYPE_STRING,
                             data,
                             len) != NVDM_STATUS_OK) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}
