/*
 * A/B control block read/write via HAL flash.
 */

#include "boot_bank_target.h"
#include "hal_flash.h"
#include "memory_map.h"
#include <string.h>

static int boot_bank_read_ctrl(boot_control_block_t *ctrl)
{
    hal_flash_status_t status;

    status = hal_flash_read(
        DUAL_IMAGE_CTRL_BASE - ROM_BASE,
        (uint8_t *)ctrl,
        sizeof(*ctrl));

    return (status == HAL_FLASH_STATUS_OK) ? 0 : -1;
}

static int boot_bank_write_ctrl(const boot_control_block_t *ctrl)
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

boot_bank_t boot_bank_query_active(void)
{
    boot_control_block_t ctrl;

    if (boot_bank_read_ctrl(&ctrl) != 0) {
        return BOOT_BANK_A;
    }

    return boot_bank_resolve(&ctrl);
}

int boot_bank_switch_active(void)
{
    return boot_bank_switch_with_hash(NULL);
}

int boot_bank_switch_with_hash(const uint8_t image_hash[64])
{
    boot_control_block_t ctrl;

    if (boot_bank_read_ctrl(&ctrl) != 0) {
        return -1;
    }

    if (ctrl.magic != BOOT_CTRL_MAGIC) {
        ctrl.magic = BOOT_CTRL_MAGIC;
        ctrl.active_flag = BOOT_FLAG_A;
    }

    if (ctrl.active_flag == BOOT_FLAG_B) {
        ctrl.active_flag = BOOT_FLAG_A;
    } else {
        ctrl.active_flag = BOOT_FLAG_B;
    }

    if (image_hash != NULL) {
        memcpy(ctrl.sha512, image_hash, sizeof(ctrl.sha512));
    }

    return boot_bank_write_ctrl(&ctrl);
}
