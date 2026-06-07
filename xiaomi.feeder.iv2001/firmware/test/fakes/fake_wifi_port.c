#include <string.h>

#include "fake_wifi_port.h"

static bool s_sta_connected = true;
static bool s_sta_has_ip = true;

void fake_wifi_port_reset(void)
{
    s_sta_connected = true;
    s_sta_has_ip = true;
}

void fake_wifi_port_set_sta_up(bool connected, bool has_ip)
{
    s_sta_connected = connected;
    s_sta_has_ip = has_ip;
}

static port_err_t fake_wifi_connect(const char *ssid, const char *pass)
{
    (void)ssid;
    (void)pass;
    return PORT_OK;
}

static bool fake_wifi_is_connected(void)
{
    return s_sta_connected;
}

static port_err_t fake_wifi_get_ip(char *buf, size_t len)
{
    if (!s_sta_has_ip || buf == NULL || len < 8) {
        return PORT_ERR_IO;
    }

    strncpy(buf, "192.168.1.10", len - 1);
    buf[len - 1] = '\0';
    return PORT_OK;
}

static port_err_t fake_wifi_start_ap(const char *ssid, const char *pass, uint8_t channel)
{
    (void)ssid;
    (void)pass;
    (void)channel;
    return PORT_ERR_NOT_SUPPORTED;
}

static port_err_t fake_wifi_stop_ap(void)
{
    return PORT_ERR_NOT_SUPPORTED;
}

static const wifi_port_t s_fake_wifi_port = {
    .connect = fake_wifi_connect,
    .is_connected = fake_wifi_is_connected,
    .get_ip = fake_wifi_get_ip,
    .start_ap = fake_wifi_start_ap,
    .stop_ap = fake_wifi_stop_ap,
};

const wifi_port_t *fake_wifi_port_get(void)
{
    return &s_fake_wifi_port;
}
