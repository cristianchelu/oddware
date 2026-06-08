/*
 * OTA URL validation and MQTT cmd parsing — spec/30-processes/ota-flow.md
 */

#ifndef OTA_URL_H
#define OTA_URL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "ota_port.h"
#include "port_err.h"
#include "flash_bank_port.h"

port_err_t ota_url_validate(const char *url);

port_err_t ota_cmd_parse(const char *payload,
                         size_t len,
                         char *url_out,
                         size_t url_out_len,
                         uint8_t sha512_out[FLASH_BANK_SHA512_LEN],
                         bool *has_sha512_out);

#endif /* OTA_URL_H */
