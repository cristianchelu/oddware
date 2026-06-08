#ifndef HAL_FLASH_H
#define HAL_FLASH_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HAL_FLASH_STATUS_OK = 0,
    HAL_FLASH_STATUS_ERROR = -1,
} hal_flash_status_t;

hal_flash_status_t hal_flash_read(uint32_t address, uint8_t *buffer, uint32_t length);

#endif
