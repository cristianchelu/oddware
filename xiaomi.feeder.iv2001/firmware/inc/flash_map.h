/*
 * IV2001 dual-image FOTA flash offsets — spec/40-architecture/partition-layout.md
 *
 * Offsets are from ROM_BASE (0x08000000). Used with MTK_FOTA_DUAL_IMAGE_ENABLE.
 */

#ifndef __FLASH_MAP_H__
#define __FLASH_MAP_H__

#ifdef MTK_FOTA_DUAL_IMAGE_ENABLE

#define LOADER_LENGTH           0x00008000  /* HEAD_1 + HEAD_2 + BL before control block */
#define FOTA_CONTROL_LENGTH     0x0000A000  /* control block through Bank A start */
#define N9_RAMCODE_LENGTH       0x00000000  /* MT7682: no separate N9 image */
#define CM4_CODE_LENGTH         0x000EE000  /* 952 KB per bank */

#define LOADER_BASE             0x0
#define FOTA_CONTROL_BASE       0x00008000
#define N9_RAMCODE_BASE         (FOTA_CONTROL_BASE + FOTA_CONTROL_LENGTH)
#define CM4_CODE_BASE           N9_RAMCODE_BASE

#define FLASH_BASE              0x10000000

#endif /* MTK_FOTA_DUAL_IMAGE_ENABLE */

#endif /* __FLASH_MAP_H__ */
