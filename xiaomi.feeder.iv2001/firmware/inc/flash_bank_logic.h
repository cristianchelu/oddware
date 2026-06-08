/*
 * Pure flash bank helpers — host-tested.
 */

#ifndef FLASH_BANK_LOGIC_H
#define FLASH_BANK_LOGIC_H

#include <stdint.h>

#include "boot_bank.h"
#include "memory_map.h"

boot_bank_t flash_bank_inactive(boot_bank_t active);

uint32_t flash_bank_rom_offset(boot_bank_t bank);

#endif /* FLASH_BANK_LOGIC_H */
