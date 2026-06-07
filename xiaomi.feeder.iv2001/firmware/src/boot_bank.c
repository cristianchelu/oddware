/*
 * A/B boot bank selection — spec/40-architecture/partition-layout.md
 */

#include <stddef.h>

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
