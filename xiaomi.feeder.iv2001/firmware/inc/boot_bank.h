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

#define BOOT_MAX_ATTEMPTS      3u
#define BOOT_UNVERIFIED_SET    1u
#define BOOT_UNVERIFIED_CLEAR  0u
#define BOOT_VECTOR_SCAN_LIMIT 0x10000u
#define BOOT_VECTOR_SCAN_STEP  4u

typedef enum {
    BOOT_BANK_A = 0,
    BOOT_BANK_B = 1,
} boot_bank_t;

typedef struct {
    uint32_t magic;
    uint32_t active_flag;
    uint8_t sha512[64];
    uint32_t unverified;
    uint8_t boot_attempts;
    uint8_t reserved[3];
} boot_control_block_t;

typedef enum {
    BOOT_ATTEMPT_CONTINUE,
    BOOT_ATTEMPT_TOGGLED,
} boot_attempt_result_t;

typedef int (*boot_bank_flash_read_fn)(uint32_t offset, uint8_t *buf, uint32_t len);

/* Pure logic — host-tested; mirrors bootloader jump policy. */
boot_bank_t boot_bank_resolve(const boot_control_block_t *ctrl);

uint32_t boot_bank_load_address(boot_bank_t bank);

uint32_t boot_bank_rom_length(boot_bank_t bank);

bool boot_bank_is_unverified(const boot_control_block_t *ctrl);

uint8_t boot_bank_effective_boot_attempts(uint8_t raw);

boot_attempt_result_t boot_bank_record_boot_attempt(boot_control_block_t *ctrl);

void boot_bank_arm_unverified(boot_control_block_t *ctrl);

void boot_bank_confirm_slot(boot_control_block_t *ctrl);

/* Host-testable image probe (8-byte header read from bank base). */
bool boot_bank_image_header_valid(const uint8_t hdr[8], uint32_t rom_base);

bool boot_bank_vector_table_valid(const uint8_t hdr[8], uint32_t rom_base);

bool boot_bank_scan_vector_table(boot_bank_flash_read_fn read,
                                 uint32_t bank_rom_offset,
                                 uint32_t rom_base);

#endif /* __BOOT_BANK_H__ */
