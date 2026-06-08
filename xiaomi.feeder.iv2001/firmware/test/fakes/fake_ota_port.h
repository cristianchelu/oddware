#ifndef FAKE_OTA_PORT_H
#define FAKE_OTA_PORT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "flash_bank_port.h"
#include "ota_port.h"
#include "port_err.h"

typedef struct fake_ota_port_state {
    unsigned start_calls;
    unsigned abort_calls;
    char last_url[OTA_URL_MAX_LEN + 1];
    uint8_t last_sha512[FLASH_BANK_SHA512_LEN];
    bool last_has_sha512;
    port_err_t start_result;
    ota_status_t status;
    ota_progress_cb_t progress_cb;
    void *progress_ctx;
} fake_ota_port_state_t;

void fake_ota_port_reset(void);
void fake_ota_port_set_start_result(port_err_t result);
void fake_ota_port_set_status(ota_status_t status);
void fake_ota_port_emit_progress(const ota_progress_t *progress);
const ota_port_t *fake_ota_port_get(void);
const fake_ota_port_state_t *fake_ota_port_state(void);

#endif /* FAKE_OTA_PORT_H */
