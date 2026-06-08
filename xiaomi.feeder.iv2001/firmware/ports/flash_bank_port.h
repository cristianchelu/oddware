/*
 * Flash bank port — spec/40-architecture/ports.md
 */

#ifndef FLASH_BANK_PORT_H
#define FLASH_BANK_PORT_H

#include <stddef.h>
#include <stdint.h>

#include "boot_bank.h"
#include "port_err.h"

#define FLASH_BANK_SHA512_LEN 64

typedef struct flash_bank_port {
    boot_bank_t (*get_active_bank)(void);
    port_err_t (*erase_inactive)(void);
    port_err_t (*write_inactive)(uint32_t offset, const uint8_t *data, size_t len);
    port_err_t (*verify_inactive)(const uint8_t expected_hash[FLASH_BANK_SHA512_LEN],
                                  uint32_t image_len);
    port_err_t (*swap_banks)(const uint8_t image_hash[FLASH_BANK_SHA512_LEN]);
} flash_bank_port_t;

const flash_bank_port_t *flash_bank_port_get(void);

#endif /* FLASH_BANK_PORT_H */
