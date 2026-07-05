IC_CONFIG                           = mt7682
BOARD_CONFIG                        = iv2001

# debug level: none, error, warning, and info
MTK_DEBUG_LEVEL                     = info

# Application log compile-time floor (debug, info, warn, error)
APP_LOG_LEVEL                         = debug

# LinkIt MQTT client logs every yield/read at info — floods UART when connected.
# Set y only for deep MQTT stack debugging (rebuild after change).
MTK_MQTT_DEBUG_ENABLE               = n

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
MTK_PING_OUT_ENABLE                 = n

# MQTT client (plain TCP; TLS deferred)
MTK_MBEDTLS_CONFIG_FILE             = config-mtk-basic.h

# HTTP client for OTA download
MTK_HTTPCLIENT_SSL_ENABLE           = n

# SNTP for civil time sync — spec/30-processes/time-sync.md
MTK_LWIP_ENABLE_SNTP                = y

# UART flash self-test on first OTA write (128/256/2048 B); set y to validate on device
FLASH_BANK_OTA_SELFTEST             = n

# Dev-only TCP console on port 2323 (bare TCP, single client). Flip to n before prod release.
REMOTE_CLI_ENABLE                   = y

# LAN admin web UI on STA (schedule, dispense, settings). Flip to n before prod release.
WEB_UI_ENABLE                       = y

# AW9523B GPIO expander on I2C1 (display rail, motor, sensors)
MTK_HAL_I2C_MASTER_ENABLE           = y
