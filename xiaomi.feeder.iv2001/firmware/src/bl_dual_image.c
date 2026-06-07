/*
 * Bootloader A/B bank jump — reads control block, no full FOTA middleware.
 */

#include "bl_dual_image.h"
#include "boot_bank.h"
#include "hal_flash.h"
#include "memory_map.h"

uint32_t bl_dual_image_boot_addr(void)
{
    boot_control_block_t ctrl;
    hal_flash_status_t status;

    status = hal_flash_read(
        DUAL_IMAGE_CTRL_BASE - ROM_BASE,
        (uint8_t *)&ctrl,
        sizeof(ctrl));

    if (status != HAL_FLASH_STATUS_OK) {
        return CM4_BASE;
    }

    return boot_bank_load_address(boot_bank_resolve(&ctrl));
}
