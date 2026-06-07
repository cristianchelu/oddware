/*
 * Wi-Fi credential validation and NVDM storage.
 * spec/30-processes/uart-console.md (Wi-Fi credential rules)
 */

#include <string.h>

#include "wifi_cred.h"
#include "config_keys.h"

static bool ssid_length_ok(const char *ssid)
{
    size_t len;

    if (ssid == NULL || ssid[0] == '\0') {
        return false;
    }

    len = strlen(ssid);
    return len > 0 && len <= WIFI_SSID_MAX_LEN;
}

static bool pass_length_ok(const char *pass)
{
    size_t len;

    if (pass == NULL) {
        return false;
    }

    len = strlen(pass);
    if (len == 0) {
        return true;
    }

    return len >= WIFI_PASS_MIN_LEN && len <= WIFI_PASS_MAX_LEN;
}

port_err_t wifi_cred_validate(const char *ssid, const char *pass)
{
    if (!ssid_length_ok(ssid) || !pass_length_ok(pass)) {
        return PORT_ERR_INVALID_ARG;
    }

    return PORT_OK;
}

port_err_t wifi_cred_save(const config_port_t *cfg, const char *ssid, const char *pass)
{
    port_err_t err;

    if (cfg == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = wifi_cred_validate(ssid, pass);
    if (err != PORT_OK) {
        return err;
    }

    if (cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_SSID, ssid) != PORT_OK) {
        return PORT_ERR_IO;
    }

    if (cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_PASS, pass) != PORT_OK) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

port_err_t wifi_cred_load(const config_port_t *cfg,
                          char *ssid, size_t ssid_len,
                          char *pass, size_t pass_len)
{
    port_err_t err;

    if (cfg == NULL || ssid == NULL || pass == NULL || ssid_len == 0 || pass_len == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    ssid[0] = '\0';
    pass[0] = '\0';

    err = cfg->read(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_SSID, ssid, ssid_len);
    if (err != PORT_OK) {
        return err;
    }

    err = cfg->read(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_PASS, pass, pass_len);
    if (err == PORT_ERR_NOT_FOUND) {
        pass[0] = '\0';
    } else if (err != PORT_OK) {
        ssid[0] = '\0';
        return err;
    }

    return wifi_cred_validate(ssid, pass);
}

bool wifi_cred_is_open_network(const char *pass)
{
    return pass != NULL && pass[0] == '\0';
}

bool wifi_cred_is_stored(const config_port_t *cfg)
{
    char ssid[WIFI_SSID_MAX_LEN + 1];

    if (cfg == NULL) {
        return false;
    }

    if (cfg->read(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_SSID, ssid, sizeof(ssid)) != PORT_OK) {
        return false;
    }

    return ssid[0] != '\0';
}
