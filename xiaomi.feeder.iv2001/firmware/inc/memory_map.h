/*
 * IV2001 flash partition constants — spec/40-architecture/partition-layout.md
 */

#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__

#define ROM_BASE 0x08000000

/* Bootloader custom_blconfig.c */
#define RAM_BASE    0x00000000
#define RAM_LENGTH  0x00400000

#define HEAD_1_BASE    0x08000000
#define HEAD_1_LENGTH  0x00001000  /* 4 KB */

#define HEAD_2_BASE    0x08001000
#define HEAD_2_LENGTH  0x00001000  /* 4 KB */

#define BL_BASE        0x08002000
#define BL_LENGTH      0x00010000  /* 64 KB */

/* Bank A — default application slot */
#define CM4_BASE       0x08012000
#define CM4_LENGTH     0x000EE000  /* 952 KB */

/* Bank B — inactive / OTA target */
#define BANK_B_BASE    0x08100000
#define BANK_B_LENGTH  0x000EE000  /* 952 KB */

/* Bootloader bl_fota.c compatibility (staging = inactive bank) */
#define FOTA_RESERVED_BASE    BANK_B_BASE
#define FOTA_RESERVED_LENGTH  BANK_B_LENGTH

/* A/B control block (inside bootloader region) */
#define DUAL_IMAGE_CTRL_BASE  0x08008000

#define ROM_NVDM_BASE           0x081EE000
#define ROM_NVDM_LENGTH         0x00010000  /* 64 KB */

#define ROM_WIFI_TX_POWER_BASE    0x081FE000
#define ROM_WIFI_TX_POWER_LENGTH  0x00001000  /* 4 KB */

#define ROM_FLASH_PAD_BASE        0x081FF000
#define ROM_FLASH_PAD_LENGTH      0x00001000  /* 4 KB — pad to 2 MB */

#define FLASH_END                 0x08200000

/* CM4 virtual memory window (SDK MPU layout — not external PSRAM) */
#define VRAM_BASE      0x10000000
#define VRAM_LENGTH    0x00400000  /* 4096 KB */

#define TCM_BASE       0x04008000
#define TCM_LENGTH     0x00010000  /* 64 KB */

#define VSYSRAM_BASE   0x14200000
#define VSYSRAM_LENGTH 0x00060000  /* 384 KB */

#endif /* __MEMORY_MAP_H__ */
