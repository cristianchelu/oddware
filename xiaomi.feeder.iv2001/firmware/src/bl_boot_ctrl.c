/*
 * Default A/B control block baked into the bootloader image.
 */

#include "boot_bank.h"

__attribute__((section(".boot_ctrl"), used))
const boot_control_block_t bl_boot_ctrl_default = {
    .magic = BOOT_CTRL_MAGIC,
    .active_flag = BOOT_FLAG_A,
};
