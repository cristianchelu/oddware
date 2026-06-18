#include <string.h>

#include "hal_flash.h"
#include "mqtt_adapter.h"
#include "wifi_api.h"

hal_flash_status_t hal_flash_read(uint32_t address, uint8_t *buffer, uint32_t length)
{
    (void)address;
    (void)buffer;
    (void)length;
    return HAL_FLASH_STATUS_ERROR;
}

void mqtt_adapter_yield(int timeout_ms)
{
    (void)timeout_ms;
}

int wifi_config_get_mac_address(wifi_hal_port_t port, uint8_t mac[6])
{
    static const uint8_t k_mac[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    (void)port;

    if (mac == NULL) {
        return -1;
    }

    memcpy(mac, k_mac, sizeof(k_mac));
    return 0;
}
