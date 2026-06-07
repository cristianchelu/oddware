/*
 * NVDM config port adapter — spec/40-architecture/ports.md
 */

#include <string.h>

#include "nvdm.h"

#include "config_port.h"

static port_err_t map_nvdm_status(nvdm_status_t status)
{
    switch (status) {
    case NVDM_STATUS_OK:
        return PORT_OK;
    case NVDM_STATUS_ITEM_NOT_FOUND:
        return PORT_ERR_NOT_FOUND;
    case NVDM_STATUS_INVALID_PARAMETER:
        return PORT_ERR_INVALID_ARG;
    default:
        return PORT_ERR_IO;
    }
}

static port_err_t nvdm_port_read(const char *group, const char *key, char *buf, size_t len)
{
    uint32_t size = (uint32_t)len;
    nvdm_status_t status;

    if (group == NULL || key == NULL || buf == NULL || len == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    status = nvdm_read_data_item(group, key, (uint8_t *)buf, &size);
    if (status != NVDM_STATUS_OK) {
        return map_nvdm_status(status);
    }

    if (size >= len) {
        buf[len - 1] = '\0';
    } else {
        buf[size] = '\0';
    }

    return PORT_OK;
}

static port_err_t nvdm_port_write(const char *group, const char *key, const char *value)
{
    if (group == NULL || key == NULL || value == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return map_nvdm_status(nvdm_write_data_item(group,
                                                key,
                                                NVDM_DATA_ITEM_TYPE_STRING,
                                                (const uint8_t *)value,
                                                (uint32_t)strlen(value) + 1));
}

static port_err_t nvdm_port_erase(const char *group, const char *key)
{
    if (group == NULL || key == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return map_nvdm_status(nvdm_delete_data_item(group, key));
}

static port_err_t nvdm_port_erase_group(const char *group)
{
    if (group == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return map_nvdm_status(nvdm_delete_group(group));
}

static const config_port_t s_nvdm_port = {
    .read = nvdm_port_read,
    .write = nvdm_port_write,
    .erase = nvdm_port_erase,
    .erase_group = nvdm_port_erase_group,
};

const config_port_t *config_port_get(void)
{
    return &s_nvdm_port;
}
