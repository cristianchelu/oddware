/*
 * OTA image helpers — spec/30-processes/ota-flow.md
 */

#ifndef OTA_IMAGE_H
#define OTA_IMAGE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "memory_map.h"
#include "port_err.h"

#define OTA_CHUNK_SIZE        4096
#define OTA_RANGE_SIZE        32768
#define OTA_RANGE_DELAY_MS    0
#define OTA_PROGRESS_STEP_PCT 5

uint8_t ota_progress_pct(uint32_t downloaded, uint32_t total);

bool ota_image_size_allowed(uint32_t size);

port_err_t ota_image_check_vector_table_at(uint32_t rom_base,
                                           const uint8_t *image,
                                           size_t len);

port_err_t ota_image_check_vector_table(const uint8_t *image, size_t len);

port_err_t ota_image_check_vector_table_in_bank(uint32_t bank_rom_offset);

#endif /* OTA_IMAGE_H */
