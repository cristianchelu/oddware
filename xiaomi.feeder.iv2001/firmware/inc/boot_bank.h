/*
 * A/B boot bank selection — spec/40-architecture/partition-layout.md
 */

#ifndef __BOOT_BANK_H__
#define __BOOT_BANK_H__

#include <stdbool.h>
#include <stdint.h>
#include "memory_map.h"

#define BOOT_CTRL_MAGIC   0x4455414Cu  /* "DUAL" */
#define BOOT_FLAG_A       0xABCDDCBAu
#define BOOT_FLAG_B       0x54322345u  /* ~BOOT_FLAG_A */
#define BOOT_FLAG_ERASED  0xFFFFFFFFu

typedef enum {
    BOOT_BANK_A = 0,
    BOOT_BANK_B = 1,
} boot_bank_t;

typedef struct {
    uint32_t magic;
    uint32_t active_flag;
    uint8_t sha512[64];
} boot_control_block_t;

/* Pure logic — host-tested; mirrors bootloader jump policy. */
boot_bank_t boot_bank_resolve(const boot_control_block_t *ctrl);

uint32_t boot_bank_load_address(boot_bank_t bank);

uint32_t boot_bank_rom_length(boot_bank_t bank);

/* Host-testable image probe (8-byte header read from bank base). */
bool boot_bank_image_header_valid(const uint8_t hdr[8], uint32_t rom_base);

#endif /* __BOOT_BANK_H__ */
