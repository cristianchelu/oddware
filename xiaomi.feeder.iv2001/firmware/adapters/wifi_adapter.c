/*
 * WiFi port adapter — spec/40-architecture/ports.md
 */

#include <string.h>

#include "wifi_api.h"
#include "wifi_lwip_helper.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "ethernetif.h"

#include "wifi_adapter.h"
#include "wifi_port.h"
#include "wifi_private_api.h"

void wifi_adapter_stack_init(void)
{
    wifi_config_t config;
    wifi_config_ext_t ext;

    memset(&config, 0, sizeof(config));
    memset(&ext, 0, sizeof(ext));

    config.opmode = WIFI_MODE_STA_ONLY;
    ext.sta_auto_connect_present = 1;
    ext.sta_auto_connect = 0;

    wifi_init(&config, &ext);
    lwip_network_init(config.opmode);
    lwip_net_start(config.opmode);
}

static port_err_t wifi_port_connect(const char *ssid, const char *pass)
{
    uint8_t ssid_len;
    uint8_t pass_len;

    if (ssid == NULL || pass == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    ssid_len = (uint8_t)strlen(ssid);
    pass_len = (uint8_t)strlen(pass);

    if (ssid_len == 0 || ssid_len > WIFI_MAX_LENGTH_OF_SSID) {
        return PORT_ERR_INVALID_ARG;
    }

    if (pass_len > WIFI_LENGTH_PASSPHRASE) {
        return PORT_ERR_INVALID_ARG;
    }

    if (wifi_config_set_ssid(WIFI_PORT_STA, (uint8_t *)ssid, ssid_len) < 0) {
        return PORT_ERR_IO;
    }

    if (pass_len > 0) {
        if (wifi_config_set_wpa_psk_key(WIFI_PORT_STA, (uint8_t *)pass, pass_len) < 0) {
            return PORT_ERR_IO;
        }
    } else if (wifi_config_set_security_mode(WIFI_PORT_STA,
                                               WIFI_AUTH_MODE_OPEN,
                                               WIFI_ENCRYPT_TYPE_WEP_DISABLED) < 0) {
        return PORT_ERR_IO;
    }

    if (wifi_config_reload_setting() < 0) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static bool wifi_port_is_connected(void)
{
    uint8_t link_status = 0;

    if (wifi_connection_get_link_status(&link_status) < 0) {
        return false;
    }

    return link_status != 0;
}

static port_err_t wifi_port_get_ip(char *buf, size_t len)
{
    struct netif *sta_if;

    if (buf == NULL || len == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    sta_if = netif_find_by_type(NETIF_TYPE_STA);
    if (sta_if == NULL || ip4_addr_isany_val(sta_if->ip_addr)) {
        return PORT_ERR_NOT_FOUND;
    }

    {
        const char *ip = inet_ntoa(sta_if->ip_addr);

        if (ip == NULL) {
            return PORT_ERR_IO;
        }

        if (strlen(ip) + 1 > len) {
            return PORT_ERR_INVALID_ARG;
        }

        strcpy(buf, ip);
    }

    return PORT_OK;
}

static port_err_t wifi_port_start_ap(const char *ssid, const char *pass, uint8_t channel)
{
    (void)ssid;
    (void)pass;
    (void)channel;
    return PORT_ERR_NOT_SUPPORTED;
}

static port_err_t wifi_port_stop_ap(void)
{
    return PORT_ERR_NOT_SUPPORTED;
}

static const wifi_port_t s_wifi_port = {
    .connect = wifi_port_connect,
    .is_connected = wifi_port_is_connected,
    .get_ip = wifi_port_get_ip,
    .start_ap = wifi_port_start_ap,
    .stop_ap = wifi_port_stop_ap,
};

const wifi_port_t *wifi_port_get(void)
{
    return &s_wifi_port;
}
