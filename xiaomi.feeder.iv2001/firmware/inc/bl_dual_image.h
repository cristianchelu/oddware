/*
 * Bootloader A/B jump helper — spec/40-architecture/partition-layout.md
 */

#ifndef __BL_DUAL_IMAGE_H__
#define __BL_DUAL_IMAGE_H__

#include <stdint.h>

uint32_t bl_dual_image_boot_addr(void);

#endif /* __BL_DUAL_IMAGE_H__ */
