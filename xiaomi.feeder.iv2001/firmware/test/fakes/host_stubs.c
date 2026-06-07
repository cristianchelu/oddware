#include <string.h>

#include "mqtt_adapter.h"
#include "wifi_api.h"

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
