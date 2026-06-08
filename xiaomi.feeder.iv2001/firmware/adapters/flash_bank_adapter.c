/*
 * Flash bank port adapter — HAL flash + boot control block.
 */

#include <stdio.h>
#include <string.h>

#include "mbedtls/sha512.h"

#include "boot_bank_target.h"
#include "flash_bank_logic.h"
#include "flash_bank_port.h"
#include "hal_flash.h"
#include "hal_flash_disk_internal.h"
#include "hal_flash_mtd_sf_dal.h"
#include "hal_flash_sf.h"
#include "memory_map.h"
#include "ota_image.h"

extern NOR_FLASH_DISK_Data EntireDiskDriveData;

static uint32_t s_erase_frontier;

/* W25Q16DW 7.2.20: 256-byte page; MTK SFC GPRAM caps MAC payload at 128 B */
#define WINBOND_PAGE_SIZE       256
#define MTK_PROG_PAYLOAD_MAX    128  /* MTD issues up to two per Winbond page */

static boot_bank_t flash_bank_adapter_get_active(void)
{
    return boot_bank_query_active();
}

extern void retrieve_FDM_lock(void);
extern void release_FDM_lock(void);

static void flash_bank_adapter_unprotect(void)
{
    SF_MTD_Data *D = (SF_MTD_Data *)EntireDiskDriveData.MTDData;
    uint8_t sr1, sr2;
    uint8_t cmd;

    retrieve_FDM_lock();

    cmd = 0x05;
    SFI_Dev_Command_Ext(D->CS, &cmd, &sr1, 1, 1);
    cmd = 0x35;
    SFI_Dev_Command_Ext(D->CS, &cmd, &sr2, 1, 1);

    if ((sr1 & 0x7C) || (sr2 & 0x40)) {
        uint8_t new_sr1 = sr1 & ~0x7C;
        uint8_t new_sr2 = sr2 & ~0x40;

        SFI_Dev_Command(D->CS, 0x06);
        SFI_Dev_Command(D->CS, (0x01 << 8) | new_sr1);

        do {
            cmd = 0x05;
            SFI_Dev_Command_Ext(D->CS, &cmd, &sr1, 1, 1);
        } while (sr1 & 0x01);

        SFI_Dev_Command(D->CS, 0x06);
        SFI_Dev_Command(D->CS, (0x31 << 8) | new_sr2);

        do {
            cmd = 0x05;
            SFI_Dev_Command_Ext(D->CS, &cmd, &sr1, 1, 1);
        } while (sr1 & 0x01);
    }

    release_FDM_lock();
}

#if defined(FLASH_BANK_OTA_SELFTEST)
static void flash_bank_adapter_selftest(uint32_t base)
{
    uint8_t buf[2048];
    uint8_t probe[4];
    hal_flash_status_t st;
    size_t i;

    for (i = 0; i < sizeof(buf); i++) {
        buf[i] = (uint8_t)(i + 1);
    }

    hal_flash_erase(base, HAL_FLASH_BLOCK_4K);

    st = hal_flash_write(base, buf, 128);
    hal_flash_read(base, probe, 4);
    printf("[flash] t128 rc=%d @0=%02x%02x%02x%02x\r\n",
           (int)st, probe[0], probe[1], probe[2], probe[3]);

    hal_flash_erase(base, HAL_FLASH_BLOCK_4K);

    st = hal_flash_write(base, buf, 256);
    hal_flash_read(base, probe, 4);
    printf("[flash] t256 rc=%d @0=%02x%02x%02x%02x",
           (int)st, probe[0], probe[1], probe[2], probe[3]);
    hal_flash_read(base + 252, probe, 4);
    printf(" @252=%02x%02x%02x%02x\r\n",
           probe[0], probe[1], probe[2], probe[3]);

    hal_flash_erase(base, HAL_FLASH_BLOCK_4K);
    hal_flash_erase(base + 0x1000, HAL_FLASH_BLOCK_4K);

    st = hal_flash_write(base, buf, 2048);
    hal_flash_read(base + 128, probe, 4);
    printf("[flash] t2048 rc=%d @128=%02x%02x%02x%02x\r\n",
           (int)st, probe[0], probe[1], probe[2], probe[3]);

    hal_flash_erase(base, HAL_FLASH_BLOCK_4K);
    hal_flash_erase(base + 0x1000, HAL_FLASH_BLOCK_4K);
}
#endif

static port_err_t flash_bank_adapter_erase_inactive(void)
{
    flash_bank_adapter_unprotect();
    s_erase_frontier = 0;
    return PORT_OK;
}

static port_err_t flash_bank_adapter_erase_ahead(uint32_t base,
                                                  uint32_t offset,
                                                  size_t len)
{
    uint32_t need_up_to = (offset + len + 0xFFFu) & ~0xFFFu;
    uint32_t sector;

    if (need_up_to <= s_erase_frontier) {
        return PORT_OK;
    }

    sector = s_erase_frontier;
    for (; sector < need_up_to; sector += 0x1000u) {
        if (hal_flash_erase(base + sector, HAL_FLASH_BLOCK_4K) != HAL_FLASH_STATUS_OK) {
            return PORT_ERR_IO;
        }
    }

    s_erase_frontier = need_up_to;
    return PORT_OK;
}

static port_err_t flash_bank_adapter_write_inactive(uint32_t offset,
                                                     const uint8_t *data,
                                                     size_t len)
{
    boot_bank_t inactive;
    uint32_t base;
    hal_flash_status_t status;
    port_err_t err;
    size_t done = 0;

    if (data == NULL || len == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    if (offset + len > CM4_LENGTH) {
        return PORT_ERR_INVALID_ARG;
    }

    inactive = flash_bank_inactive(flash_bank_adapter_get_active());
    base = flash_bank_rom_offset(inactive);

    err = flash_bank_adapter_erase_ahead(base, offset, len);
    if (err != PORT_OK) {
        return err;
    }

#if defined(FLASH_BANK_OTA_SELFTEST)
    if (offset == 0) {
        flash_bank_adapter_selftest(base);
    }
#endif

    while (done < len) {
        uint32_t addr = base + offset + done;
        size_t page_remain = WINBOND_PAGE_SIZE - (addr & (WINBOND_PAGE_SIZE - 1));
        size_t chunk = len - done;

        if (chunk > page_remain) {
            chunk = page_remain;
        }

        status = hal_flash_write(addr, (uint8_t *)data + done, chunk);
        if (status != HAL_FLASH_STATUS_OK) {
            return PORT_ERR_IO;
        }
        done += chunk;
    }

    return PORT_OK;
}

static port_err_t flash_bank_adapter_verify_inactive(const uint8_t expected_hash[FLASH_BANK_SHA512_LEN],
                                                     uint32_t image_len)
{
    boot_bank_t inactive;
    uint32_t offset;
    uint32_t base;
    uint8_t chunk[OTA_CHUNK_SIZE];
    uint32_t remaining;
    mbedtls_sha512_context ctx;
    uint8_t computed[FLASH_BANK_SHA512_LEN];
    hal_flash_status_t status;

    if (expected_hash == NULL || !ota_image_size_allowed(image_len)) {
        return PORT_ERR_INVALID_ARG;
    }

    inactive = flash_bank_inactive(flash_bank_adapter_get_active());
    base = flash_bank_rom_offset(inactive);
    remaining = image_len;

    mbedtls_sha512_init(&ctx);
    mbedtls_sha512_starts(&ctx, 0);

    offset = 0;
    while (remaining > 0) {
        size_t chunk_len = remaining;

        if (chunk_len > sizeof(chunk)) {
            chunk_len = sizeof(chunk);
        }

        status = hal_flash_read(base + offset, chunk, chunk_len);
        if (status != HAL_FLASH_STATUS_OK) {
            mbedtls_sha512_free(&ctx);
            return PORT_ERR_IO;
        }

        mbedtls_sha512_update(&ctx, chunk, chunk_len);
        offset += (uint32_t)chunk_len;
        remaining -= (uint32_t)chunk_len;
    }

    mbedtls_sha512_finish(&ctx, computed);
    mbedtls_sha512_free(&ctx);

    if (memcmp(computed, expected_hash, FLASH_BANK_SHA512_LEN) != 0) {
        return PORT_ERR_INVALID_ARG;
    }

    return PORT_OK;
}

static port_err_t flash_bank_adapter_swap_banks(const uint8_t image_hash[FLASH_BANK_SHA512_LEN])
{
    if (boot_bank_switch_with_hash(image_hash) != 0) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static const flash_bank_port_t s_flash_bank_port = {
    .get_active_bank = flash_bank_adapter_get_active,
    .erase_inactive = flash_bank_adapter_erase_inactive,
    .write_inactive = flash_bank_adapter_write_inactive,
    .verify_inactive = flash_bank_adapter_verify_inactive,
    .swap_banks = flash_bank_adapter_swap_banks,
};

const flash_bank_port_t *flash_bank_port_get(void)
{
    return &s_flash_bank_port;
}
