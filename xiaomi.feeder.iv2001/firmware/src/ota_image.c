/*
 * OTA image helpers — spec/30-processes/ota-flow.md
 */

#include "hal_flash.h"

#include "ota_image.h"

#define OTA_VECTOR_SCAN_LIMIT 0x10000u
#define OTA_VECTOR_SCAN_STEP  4u

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
    uint32_t sp;
    uint32_t reset;

    if (image == NULL || len < 8) {
        return PORT_ERR_INVALID_ARG;
    }

    sp = (uint32_t)image[0] |
         ((uint32_t)image[1] << 8) |
         ((uint32_t)image[2] << 16) |
         ((uint32_t)image[3] << 24);
    reset = (uint32_t)image[4] |
            ((uint32_t)image[5] << 8) |
            ((uint32_t)image[6] << 16) |
            ((uint32_t)image[7] << 24);

    if (!((sp >= VSYSRAM_BASE && sp <= (VSYSRAM_BASE + VSYSRAM_LENGTH)) ||
          (sp >= TCM_BASE && sp <= (TCM_BASE + TCM_LENGTH)))) {
        return PORT_ERR_INVALID_ARG;
    }

    if ((reset & 1u) == 0u) {
        return PORT_ERR_INVALID_ARG;
    }

    if (reset < rom_base || reset >= (rom_base + CM4_LENGTH)) {
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
    uint8_t hdr[8];
    uint32_t off;
    uint32_t rom_base = ROM_BASE + bank_rom_offset;

    for (off = 0; off < OTA_VECTOR_SCAN_LIMIT; off += OTA_VECTOR_SCAN_STEP) {
        if (hal_flash_read(bank_rom_offset + off, hdr, sizeof(hdr)) !=
            HAL_FLASH_STATUS_OK) {
            return PORT_ERR_IO;
        }

        if (ota_image_check_vector_table_at(rom_base, hdr, sizeof(hdr)) == PORT_OK) {
            return PORT_OK;
        }
    }

    return PORT_ERR_INVALID_ARG;
}
