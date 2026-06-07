/*
 * MQTT broker config validation and NVDM storage.
 * spec/30-processes/uart-console.md (MQTT broker rules)
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mqtt_cred.h"
#include "mqtt_topics.h"
#include "config_keys.h"

static bool host_length_ok(const char *host)
{
    size_t len;

    if (host == NULL || host[0] == '\0') {
        return false;
    }

    len = strlen(host);
    return len > 0 && len <= MQTT_HOST_MAX_LEN;
}

static bool device_id_chars_ok(const char *device_id)
{
    size_t i;
    size_t len;

    if (device_id == NULL) {
        return false;
    }

    len = strlen(device_id);
    if (len == 0) {
        return true;
    }

    if (len > MQTT_DEVICE_ID_MAX_LEN) {
        return false;
    }

    for (i = 0; i < len; i++) {
        char c = device_id[i];

        if (isalnum((unsigned char)c) || c == '_' || c == '-') {
            continue;
        }
        return false;
    }

    return true;
}

static bool parse_bool(const char *value, bool *out)
{
    if (value == NULL || out == NULL) {
        return false;
    }

    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
        *out = true;
        return true;
    }

    if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
        *out = false;
        return true;
    }

    return false;
}

port_err_t mqtt_cred_validate_host(const char *host)
{
    return host_length_ok(host) ? PORT_OK : PORT_ERR_INVALID_ARG;
}

port_err_t mqtt_cred_validate_port(uint16_t port)
{
    if (port < 1) {
        return PORT_ERR_INVALID_ARG;
    }

    return PORT_OK;
}

port_err_t mqtt_cred_validate_device_id(const char *device_id)
{
    return device_id_chars_ok(device_id) ? PORT_OK : PORT_ERR_INVALID_ARG;
}

port_err_t mqtt_cred_save_host(const config_port_t *cfg, const char *host)
{
    if (cfg == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (mqtt_cred_validate_host(host) != PORT_OK) {
        return PORT_ERR_INVALID_ARG;
    }

    return cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, host);
}

port_err_t mqtt_cred_save_port(const config_port_t *cfg, uint16_t port)
{
    char buf[8];

    if (cfg == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (mqtt_cred_validate_port(port) != PORT_OK) {
        return PORT_ERR_INVALID_ARG;
    }

    snprintf(buf, sizeof(buf), "%u", (unsigned)port);
    return cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PORT, buf);
}

port_err_t mqtt_cred_save_user(const config_port_t *cfg, const char *user)
{
    if (cfg == NULL || user == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (strlen(user) > MQTT_USER_MAX_LEN) {
        return PORT_ERR_INVALID_ARG;
    }

    return cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_USER, user);
}

port_err_t mqtt_cred_save_pass(const config_port_t *cfg, const char *pass)
{
    if (cfg == NULL || pass == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (strlen(pass) > MQTT_PASS_MAX_LEN) {
        return PORT_ERR_INVALID_ARG;
    }

    return cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PASS, pass);
}

port_err_t mqtt_cred_save_device_id(const config_port_t *cfg, const char *device_id)
{
    if (cfg == NULL || device_id == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (mqtt_cred_validate_device_id(device_id) != PORT_OK) {
        return PORT_ERR_INVALID_ARG;
    }

    if (device_id[0] == '\0') {
        return cfg->erase(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_DEVICE_ID);
    }

    return cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_DEVICE_ID, device_id);
}

port_err_t mqtt_cred_load(const config_port_t *cfg, mqtt_cred_t *out)
{
    char port_buf[8];
    char tls_buf[8];
    port_err_t err;
    bool tls = false;
    unsigned long port_val;

    if (cfg == NULL || out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));
    out->port = 1883;

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, out->host, sizeof(out->host));
    if (err != PORT_OK) {
        return err;
    }

    if (mqtt_cred_validate_host(out->host) != PORT_OK) {
        out->host[0] = '\0';
        return PORT_ERR_INVALID_ARG;
    }

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PORT, port_buf, sizeof(port_buf));
    if (err == PORT_OK) {
        port_val = strtoul(port_buf, NULL, 10);
        if (port_val < 1 || port_val > 65535) {
            return PORT_ERR_INVALID_ARG;
        }
        out->port = (uint16_t)port_val;
    } else if (err != PORT_ERR_NOT_FOUND) {
        return err;
    }

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_USER, out->user, sizeof(out->user));
    if (err == PORT_ERR_NOT_FOUND) {
        out->user[0] = '\0';
    } else if (err != PORT_OK) {
        return err;
    }

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PASS, out->pass, sizeof(out->pass));
    if (err == PORT_ERR_NOT_FOUND) {
        out->pass[0] = '\0';
    } else if (err != PORT_OK) {
        return err;
    }

    err = cfg->read(CONFIG_GROUP_MQTT,
                    CONFIG_KEY_MQTT_DEVICE_ID,
                    out->device_id,
                    sizeof(out->device_id));
    if (err == PORT_ERR_NOT_FOUND) {
        out->device_id[0] = '\0';
    } else if (err != PORT_OK) {
        return err;
    } else if (mqtt_cred_validate_device_id(out->device_id) != PORT_OK) {
        return PORT_ERR_INVALID_ARG;
    }

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_TLS, tls_buf, sizeof(tls_buf));
    if (err == PORT_OK) {
        if (!parse_bool(tls_buf, &tls)) {
            return PORT_ERR_INVALID_ARG;
        }
        out->tls = tls;
    } else if (err != PORT_ERR_NOT_FOUND) {
        return err;
    }

    if (out->tls) {
        return PORT_ERR_NOT_SUPPORTED;
    }

    return PORT_OK;
}

bool mqtt_cred_is_stored(const config_port_t *cfg)
{
    char host[MQTT_HOST_MAX_LEN + 1];

    if (cfg == NULL) {
        return false;
    }

    if (cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, host, sizeof(host)) != PORT_OK) {
        return false;
    }

    return host[0] != '\0';
}

void mqtt_cred_resolve_device_id(const mqtt_cred_t *cred,
                                 const char *mac_hex12,
                                 char *buf,
                                 size_t len)
{
    if (buf == NULL || len == 0) {
        return;
    }

    if (cred != NULL && cred->device_id[0] != '\0') {
        strncpy(buf, cred->device_id, len - 1);
        buf[len - 1] = '\0';
        return;
    }

    if (mqtt_device_id_from_mac(buf, len, mac_hex12) != PORT_OK) {
        buf[0] = '\0';
    }
}
