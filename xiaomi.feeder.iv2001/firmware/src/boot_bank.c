/*
 * A/B boot bank selection — spec/40-architecture/partition-layout.md
 */

#include <stddef.h>
#include <string.h>

#include "boot_bank.h"

boot_bank_t boot_bank_resolve(const boot_control_block_t *ctrl)
{
    if (ctrl == NULL || ctrl->magic != BOOT_CTRL_MAGIC) {
        return BOOT_BANK_A;
    }

    if (ctrl->active_flag == BOOT_FLAG_B) {
        return BOOT_BANK_B;
    }

    /* Bank A: explicit A mark, erased flash, or corrupt flag */
    return BOOT_BANK_A;
}

uint32_t boot_bank_load_address(boot_bank_t bank)
{
    return (bank == BOOT_BANK_B) ? BANK_B_BASE : CM4_BASE;
}

uint32_t boot_bank_rom_length(boot_bank_t bank)
{
    (void)bank;
    return CM4_LENGTH;
}

bool boot_bank_image_header_valid(const uint8_t hdr[8], uint32_t rom_base)
{
    uint32_t sp;
    uint32_t reset;
    uint16_t first_insn;
    size_t i;
    bool all_ff = true;
    bool all_00 = true;

    if (hdr == NULL) {
        return false;
    }

    for (i = 0; i < 8; i++) {
        if (hdr[i] != 0xFF) {
            all_ff = false;
        }
        if (hdr[i] != 0x00) {
            all_00 = false;
        }
    }

    if (all_ff || all_00) {
        return false;
    }

    sp = (uint32_t)hdr[0] |
         ((uint32_t)hdr[1] << 8) |
         ((uint32_t)hdr[2] << 16) |
         ((uint32_t)hdr[3] << 24);
    reset = (uint32_t)hdr[4] |
            ((uint32_t)hdr[5] << 8) |
            ((uint32_t)hdr[6] << 16) |
            ((uint32_t)hdr[7] << 24);

    if ((sp >= VSYSRAM_BASE && sp <= (VSYSRAM_BASE + VSYSRAM_LENGTH)) ||
        (sp >= TCM_BASE && sp <= (TCM_BASE + TCM_LENGTH))) {
        if ((reset & 1u) != 0u &&
            reset >= rom_base &&
            reset < (rom_base + CM4_LENGTH)) {
            return true;
        }
    }

    /* MTK images jump straight to Reset_Handler at the bank base. */
    first_insn = (uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8);
    if (first_insn == 0xFFFFu || first_insn == 0x0000u) {
        return false;
    }

    return true;
}
