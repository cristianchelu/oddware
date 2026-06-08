/*
 * Pure flash bank helpers — host-tested.
 */

#include "flash_bank_logic.h"

boot_bank_t flash_bank_inactive(boot_bank_t active)
{
    return (active == BOOT_BANK_A) ? BOOT_BANK_B : BOOT_BANK_A;
}

uint32_t flash_bank_rom_offset(boot_bank_t bank)
{
    return (bank == BOOT_BANK_B) ? (BANK_B_BASE - ROM_BASE) : (CM4_BASE - ROM_BASE);
}
