IC_CONFIG                           = mt7682
BOARD_CONFIG                        = iv2001

# debug level: none, error, warning, and info
MTK_DEBUG_LEVEL                     = info

MTK_NO_PSRAM_ENABLE                 = y
MTK_MEMORY_WITH_PSRAM_FLASH         = n
MTK_MEMORY_WITHOUT_PSRAM            = y
MTK_MEMORY_WITHOUT_PSRAM_FLASH      = n

MTK_HAL_LOWPOWER_ENABLE             = n
MTK_HIF_GDMA_ENABLE                 = y

# Step 2: dual-image FOTA + UART CLI bank switch
MTK_FOTA_ENABLE                     = y
MTK_FOTA_DUAL_IMAGE_ENABLE          = y
MTK_FOTA_DUAL_IMAGE_SWITCH_ONLY     = y
MTK_FOTA_CLI_ENABLE                 = n
MTK_MINICLI_ENABLE                  = y
