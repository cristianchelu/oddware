#include "nvdm_record.h"
#include "wifi_sdk_hal.h"

port_err_t wifi_sdk_hal_stop_scan(void)
{
    return PORT_OK;
}

port_err_t wifi_sdk_hal_disconnect_ap(void)
{
    return PORT_OK;
}

port_err_t wifi_sdk_hal_reload_setting(void)
{
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

    nvdm_record_write(group, key, data, len);
    return PORT_OK;
}
