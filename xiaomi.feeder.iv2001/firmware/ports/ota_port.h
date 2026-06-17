/*
 * OTA port — spec/40-architecture/ports.md
 */

#ifndef OTA_PORT_H
#define OTA_PORT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "port_err.h"

#define OTA_URL_MAX_LEN   255
#define OTA_SHA512_HEX_LEN 128

typedef enum {
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_PREPARING,
    OTA_STATUS_CONNECTING,
    OTA_STATUS_DOWNLOADING,
    OTA_STATUS_VERIFYING,
    OTA_STATUS_APPLYING,
    OTA_STATUS_ERROR,
} ota_status_t;

typedef struct ota_progress {
    ota_status_t status;
    uint8_t pct;
    const char *error;
} ota_progress_t;

typedef void (*ota_progress_cb_t)(const ota_progress_t *progress, void *ctx);

typedef struct ota_port {
    port_err_t (*start)(const char *url,
                        const uint8_t *expected_sha512,
                        bool has_expected_sha512);
    ota_status_t (*get_status)(void);
    port_err_t (*abort)(void);
    void (*set_progress_cb)(ota_progress_cb_t cb, void *ctx);
} ota_port_t;

const ota_port_t *ota_port_get(void);

#endif /* OTA_PORT_H */
