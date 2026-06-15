#include <string.h>

#include "fake_wifi_port.h"

static uint32_t s_disconnect_calls;
static uint32_t s_radio_up_calls;
static uint32_t s_connect_calls;
static uint32_t s_wait_ready_calls;
static bool s_radio_on = true;
static bool s_sta_connected = true;
static bool s_sta_has_ip = true;
static port_err_t s_wait_ready_result = PORT_OK;

void fake_wifi_port_reset(void)
{
    s_disconnect_calls = 0u;
    s_radio_up_calls = 0u;
    s_connect_calls = 0u;
    s_wait_ready_calls = 0u;
    s_radio_on = true;
    s_sta_connected = true;
    s_sta_has_ip = true;
    s_wait_ready_result = PORT_OK;
}

const fake_wifi_port_state_t *fake_wifi_port_state(void)
{
    static fake_wifi_port_state_t state;

    state.disconnect_calls = s_disconnect_calls;
    state.radio_up_calls = s_radio_up_calls;
    state.connect_calls = s_connect_calls;
    state.wait_ready_calls = s_wait_ready_calls;
    state.radio_on = s_radio_on;
    state.connected = s_sta_connected;
    state.has_ip = s_sta_has_ip;
    return &state;
}

void fake_wifi_port_set_sta_up(bool connected, bool has_ip)
{
    s_sta_connected = connected;
    s_sta_has_ip = has_ip;
}

void fake_wifi_port_set_wait_ready_result(port_err_t result)
{
    s_wait_ready_result = result;
}

static port_err_t fake_wifi_disconnect(void)
{
    s_disconnect_calls++;
    s_sta_connected = false;
    s_sta_has_ip = false;
    s_radio_on = false;
    return PORT_OK;
}

static port_err_t fake_wifi_radio_up(void)
{
    s_radio_up_calls++;
    s_radio_on = true;
    return PORT_OK;
}

static port_err_t fake_wifi_connect(const char *ssid, const char *pass)
{
    (void)ssid;
    (void)pass;
    s_connect_calls++;
    return PORT_OK;
}

static port_err_t fake_wifi_wait_ready(uint32_t timeout_ms)
{
    (void)timeout_ms;
    s_wait_ready_calls++;

    if (s_wait_ready_result != PORT_OK) {
        return s_wait_ready_result;
    }

    if (!s_radio_on) {
        return PORT_ERR_IO;
    }

    s_sta_connected = true;
    s_sta_has_ip = true;
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
    .disconnect = fake_wifi_disconnect,
    .radio_up = fake_wifi_radio_up,
    .connect = fake_wifi_connect,
    .wait_ready = fake_wifi_wait_ready,
    .is_connected = fake_wifi_is_connected,
    .get_ip = fake_wifi_get_ip,
    .start_ap = fake_wifi_start_ap,
    .stop_ap = fake_wifi_stop_ap,
};

const wifi_port_t *fake_wifi_port_get(void)
{
    return &s_fake_wifi_port;
}
