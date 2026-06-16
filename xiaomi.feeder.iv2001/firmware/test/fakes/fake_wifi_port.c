#include <string.h>

#include "fake_wifi_port.h"

static uint32_t s_disconnect_calls;
static uint32_t s_set_credentials_calls;
static uint32_t s_radio_up_calls;
static uint32_t s_arm_connect_calls;
static uint32_t s_connect_calls;
static uint32_t s_wait_ready_calls;
static fake_wifi_op_t s_op_log[FAKE_WIFI_OP_LOG_MAX];
static uint32_t s_op_log_len;
static bool s_radio_on = true;
static bool s_sta_connected = true;
static bool s_sta_has_ip = true;
static port_err_t s_wait_ready_result = PORT_OK;

static void fake_wifi_log_op(fake_wifi_op_t op)
{
    if (s_op_log_len < FAKE_WIFI_OP_LOG_MAX) {
        s_op_log[s_op_log_len++] = op;
    }
}

void fake_wifi_port_reset(void)
{
    s_disconnect_calls = 0u;
    s_set_credentials_calls = 0u;
    s_radio_up_calls = 0u;
    s_arm_connect_calls = 0u;
    s_connect_calls = 0u;
    s_wait_ready_calls = 0u;
    s_op_log_len = 0u;
    s_radio_on = true;
    s_sta_connected = true;
    s_sta_has_ip = true;
    s_wait_ready_result = PORT_OK;
}

const fake_wifi_port_state_t *fake_wifi_port_state(void)
{
    static fake_wifi_port_state_t state;

    state.disconnect_calls = s_disconnect_calls;
    state.set_credentials_calls = s_set_credentials_calls;
    state.radio_up_calls = s_radio_up_calls;
    state.arm_connect_calls = s_arm_connect_calls;
    state.connect_calls = s_connect_calls;
    state.wait_ready_calls = s_wait_ready_calls;
    memcpy(state.op_log, s_op_log, sizeof(s_op_log));
    state.op_log_len = s_op_log_len;
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
    fake_wifi_log_op(FAKE_WIFI_OP_DISCONNECT);
    s_sta_connected = false;
    s_sta_has_ip = false;
    s_radio_on = false;
    return PORT_OK;
}

static port_err_t fake_wifi_radio_up(void)
{
    s_radio_up_calls++;
    fake_wifi_log_op(FAKE_WIFI_OP_RADIO_UP);
    s_radio_on = true;
    return PORT_OK;
}

static port_err_t fake_wifi_set_credentials(const char *ssid, const char *pass)
{
    (void)ssid;
    (void)pass;
    s_set_credentials_calls++;
    fake_wifi_log_op(FAKE_WIFI_OP_SET_CREDENTIALS);
    return PORT_OK;
}

static port_err_t fake_wifi_arm_connect(void)
{
    s_arm_connect_calls++;
    fake_wifi_log_op(FAKE_WIFI_OP_ARM_CONNECT);
    return PORT_OK;
}

static port_err_t fake_wifi_connect(const char *ssid, const char *pass)
{
    port_err_t err;

    err = fake_wifi_set_credentials(ssid, pass);
    if (err != PORT_OK) {
        return err;
    }

    s_connect_calls++;
    fake_wifi_log_op(FAKE_WIFI_OP_CONNECT);
    return fake_wifi_arm_connect();
}

static port_err_t fake_wifi_wait_ready(uint32_t timeout_ms)
{
    (void)timeout_ms;
    s_wait_ready_calls++;
    fake_wifi_log_op(FAKE_WIFI_OP_WAIT_READY);

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
    .set_credentials = fake_wifi_set_credentials,
    .arm_connect = fake_wifi_arm_connect,
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
