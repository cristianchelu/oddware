/*
 * Erase invalid NVDM before SDK nvdm_init().
 * spec/40-architecture/partition-layout.md
 */

#include <stdbool.h>
#include <stdint.h>

#include "hal_flash.h"
#include "hal_platform.h"
#include "memory_map.h"

#define NVDM_PEB_MAGIC      0x4d44564eu
#define NVDM_VERSION_VAL    0x01u
#define ERASE_COUNT_MAX     0xffffffffu
#define PEB_STATUS_EMPTY    0xfeu
#define PEB_STATUS_ACTIVED  0xe0u

typedef struct {
    uint32_t magic;
    uint32_t erase_count;
    uint8_t status;
    uint8_t peb_reserved;
    uint8_t version;
} nvdm_peb_hdr_t;

static bool nvdm_peb0_is_valid(const nvdm_peb_hdr_t *hdr)
{
    if (hdr->magic != NVDM_PEB_MAGIC) {
        return false;
    }
    if (hdr->erase_count == ERASE_COUNT_MAX || hdr->version != NVDM_VERSION_VAL) {
        return false;
    }
    if (hdr->status == PEB_STATUS_EMPTY) {
        return hdr->peb_reserved == 0xffu;
    }
    if (hdr->status == PEB_STATUS_ACTIVED) {
        return hdr->peb_reserved != 0xffu;
    }
    return hdr->peb_reserved != 0xffu;
}

static void nvdm_erase_region(uint32_t offset, uint32_t length)
{
    uint32_t end = offset + length;

    for (; offset < end; offset += 0x1000u) {
        hal_flash_erase(offset, HAL_FLASH_BLOCK_4K);
    }
}

void nvdm_prepare_region(void)
{
    nvdm_peb_hdr_t hdr;
    uint32_t offset = ROM_NVDM_BASE - HAL_FLASH_BASE_ADDRESS;
    hal_flash_status_t status;

    status = hal_flash_read(offset, (uint8_t *)&hdr, sizeof(hdr));
    if (status != HAL_FLASH_STATUS_OK || !nvdm_peb0_is_valid(&hdr)) {
        nvdm_erase_region(offset, ROM_NVDM_LENGTH);
    }
}
