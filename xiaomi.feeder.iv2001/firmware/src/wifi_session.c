/*
 * Wi-Fi session orchestration — spec/30-processes/wifi-lifecycle.md
 *
 * Connect order: set_credentials → radio_up → arm_connect → wait_ready.
 */

#include "wifi_port.h"
#include "wifi_session.h"

port_err_t wifi_session_down(void)
{
    const wifi_port_t *wifi = wifi_port_get();

    if (wifi == NULL || wifi->disconnect == NULL) {
        return PORT_ERR_IO;
    }

    return wifi->disconnect();
}

port_err_t wifi_session_up(void)
{
    const wifi_port_t *wifi = wifi_port_get();

    if (wifi == NULL || wifi->radio_up == NULL) {
        return PORT_ERR_IO;
    }

    return wifi->radio_up();
}

port_err_t wifi_session_connect(const char *ssid, const char *pass, uint32_t timeout_ms)
{
    const wifi_port_t *wifi = wifi_port_get();

    if (wifi == NULL
        || wifi->set_credentials == NULL
        || wifi->radio_up == NULL
        || wifi->arm_connect == NULL
        || wifi->wait_ready == NULL) {
        return PORT_ERR_IO;
    }

    if (wifi->set_credentials(ssid, pass) != PORT_OK) {
        return PORT_ERR_IO;
    }

    if (wifi->radio_up() != PORT_OK) {
        return PORT_ERR_IO;
    }

    if (wifi->arm_connect() != PORT_OK) {
        return PORT_ERR_IO;
    }

    if (wifi->wait_ready(timeout_ms) != PORT_OK) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}
