/*
 * OTA image helpers — spec/30-processes/ota-flow.md
 */

#include "boot_bank.h"
#include "hal_flash.h"

#include "ota_image.h"

static int ota_image_flash_read(uint32_t offset, uint8_t *buf, uint32_t len)
{
    return (hal_flash_read(offset, buf, len) == HAL_FLASH_STATUS_OK) ? 0 : -1;
}

uint8_t ota_progress_pct(uint32_t downloaded, uint32_t total)
{
    uint32_t pct;

    if (total == 0) {
        return 0;
    }

    pct = (downloaded * 100u) / total;
    if (pct > 100u) {
        pct = 100u;
    }

    return (uint8_t)((pct / OTA_PROGRESS_STEP_PCT) * OTA_PROGRESS_STEP_PCT);
}

bool ota_image_size_allowed(uint32_t size)
{
    return size > 0 && size <= CM4_LENGTH;
}

port_err_t ota_image_check_vector_table_at(uint32_t rom_base,
                                           const uint8_t *image,
                                           size_t len)
{
    if (image == NULL || len < 8) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!boot_bank_vector_table_valid(image, rom_base)) {
        return PORT_ERR_INVALID_ARG;
    }

    return PORT_OK;
}

port_err_t ota_image_check_vector_table(const uint8_t *image, size_t len)
{
    return ota_image_check_vector_table_at(CM4_BASE, image, len);
}

port_err_t ota_image_check_vector_table_in_bank(uint32_t bank_rom_offset)
{
    uint32_t rom_base = ROM_BASE + bank_rom_offset;

    if (boot_bank_scan_vector_table(ota_image_flash_read, bank_rom_offset, rom_base)) {
        return PORT_OK;
    }

    return PORT_ERR_INVALID_ARG;
}
