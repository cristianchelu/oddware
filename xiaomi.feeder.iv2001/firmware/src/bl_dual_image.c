/*
 * Bootloader A/B bank jump — read control block, validate, fallback.
 */

#include <string.h>

#include "bl_common.h"
#include "bl_dual_image.h"
#include "boot_bank.h"
#include "hal_flash.h"
#include "memory_map.h"

static int bl_dual_image_read_ctrl(boot_control_block_t *ctrl)
{
    hal_flash_status_t status;

    status = hal_flash_read(
        DUAL_IMAGE_CTRL_BASE - ROM_BASE,
        (uint8_t *)ctrl,
        sizeof(*ctrl));

    return (status == HAL_FLASH_STATUS_OK) ? 0 : -1;
}

static int bl_dual_image_write_ctrl(const boot_control_block_t *ctrl)
{
    hal_flash_status_t status;

    status = hal_flash_erase(DUAL_IMAGE_CTRL_BASE - ROM_BASE, HAL_FLASH_BLOCK_4K);
    if (status != HAL_FLASH_STATUS_OK) {
        return -1;
    }

    status = hal_flash_write(
        DUAL_IMAGE_CTRL_BASE - ROM_BASE,
        (uint8_t *)ctrl,
        sizeof(*ctrl));

    return (status == HAL_FLASH_STATUS_OK) ? 0 : -1;
}

static bool bl_dual_image_bank_valid(boot_bank_t bank)
{
    uint8_t hdr[8];
    uint32_t rom_base = boot_bank_load_address(bank);
    hal_flash_status_t status;

    status = hal_flash_read(rom_base - ROM_BASE, hdr, sizeof(hdr));
    if (status != HAL_FLASH_STATUS_OK) {
        return false;
    }

    return boot_bank_image_header_valid(hdr, rom_base);
}

static void bl_dual_image_persist_bank(boot_bank_t bank)
{
    boot_control_block_t ctrl;

    if (bl_dual_image_read_ctrl(&ctrl) != 0) {
        memset(&ctrl, 0xFF, sizeof(ctrl));
    }

    ctrl.magic = BOOT_CTRL_MAGIC;
    ctrl.active_flag = (bank == BOOT_BANK_B) ? BOOT_FLAG_B : BOOT_FLAG_A;
    (void)bl_dual_image_write_ctrl(&ctrl);
}

static uint32_t bl_dual_image_pick_bank(boot_bank_t preferred, boot_bank_t fallback)
{
    if (bl_dual_image_bank_valid(preferred)) {
        return boot_bank_load_address(preferred);
    }

    bl_print(LOG_WARN, "bank %c image invalid\r\n",
             (preferred == BOOT_BANK_B) ? 'B' : 'A');

    if (bl_dual_image_bank_valid(fallback)) {
        bl_print(LOG_WARN, "fallback to bank %c\r\n",
                 (fallback == BOOT_BANK_B) ? 'B' : 'A');
        bl_dual_image_persist_bank(fallback);
        return boot_bank_load_address(fallback);
    }

    bl_print(LOG_ERROR, "both banks invalid, forcing A\r\n");
    return CM4_BASE;
}

uint32_t bl_dual_image_boot_addr(void)
{
    boot_control_block_t ctrl;
    boot_bank_t preferred;
    boot_bank_t fallback;

    if (bl_dual_image_read_ctrl(&ctrl) != 0) {
        return bl_dual_image_pick_bank(BOOT_BANK_A, BOOT_BANK_B);
    }

    preferred = boot_bank_resolve(&ctrl);
    fallback = (preferred == BOOT_BANK_A) ? BOOT_BANK_B : BOOT_BANK_A;

    return bl_dual_image_pick_bank(preferred, fallback);
}
