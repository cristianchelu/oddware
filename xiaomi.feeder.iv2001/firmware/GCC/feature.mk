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

# Dual-image FOTA + UART CLI
MTK_FOTA_ENABLE                     = y
MTK_FOTA_DUAL_IMAGE_ENABLE          = y
MTK_FOTA_DUAL_IMAGE_SWITCH_ONLY     = y
MTK_FOTA_CLI_ENABLE                 = n
MTK_MINICLI_ENABLE                  = y

# Wi-Fi STA + NVDM (sta_auto_connect disabled; app credentials in wifi/ssid + wifi/pass)
MTK_NVDM_ENABLE                     = y
MTK_WIFI_TGN_VERIFY_ENABLE          = n
MTK_WIFI_WPS_ENABLE                 = n
MTK_WIFI_DIRECT_ENABLE              = n
MTK_WIFI_REPEATER_ENABLE            = n
MTK_WIFI_PROFILE_ENABLE             = y
MTK_CM4_WIFI_TASK_ENABLE            = y
MTK_WIFI_ROM_ENABLE                 = y
MTK_PING_OUT_ENABLE                 = y
