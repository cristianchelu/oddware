/*
 * MQTT topic helpers — spec/30-processes/mqtt-protocol.md
 */

#include <stdio.h>
#include <string.h>

#include "mqtt_topics.h"

port_err_t mqtt_topic_format(char *buf,
                             size_t len,
                             const char *device_id,
                             const char *suffix)
{
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || suffix == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (device_id[0] == '\0' || suffix[0] == '\0') {
        return PORT_ERR_INVALID_ARG;
    }

    written = snprintf(buf, len, "%s/%s/%s", MQTT_TOPIC_PREFIX, device_id, suffix);
    if (written < 0 || (size_t)written >= len) {
        return PORT_ERR_INVALID_ARG;
    }

    return PORT_OK;
}

port_err_t mqtt_client_id_format(char *buf, size_t len, const char *device_id)
{
    int written;

    if (buf == NULL || len == 0 || device_id == NULL || device_id[0] == '\0') {
        return PORT_ERR_INVALID_ARG;
    }

    written = snprintf(buf, len, "%s_%s", MQTT_TOPIC_PREFIX, device_id);
    if (written < 0 || (size_t)written >= len) {
        return PORT_ERR_INVALID_ARG;
    }

    return PORT_OK;
}

port_err_t mqtt_device_id_from_mac(char *buf, size_t len, const char *mac_hex12)
{
    size_t mac_len;

    if (buf == NULL || len == 0 || mac_hex12 == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    mac_len = strlen(mac_hex12);
    if (mac_len < 6) {
        return PORT_ERR_INVALID_ARG;
    }

    if (len < 7) {
        return PORT_ERR_INVALID_ARG;
    }

    memcpy(buf, mac_hex12 + mac_len - 6, 6);
    buf[6] = '\0';
    return PORT_OK;
}
