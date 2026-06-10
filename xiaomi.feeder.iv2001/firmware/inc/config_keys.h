/*
 * NVDM namespace keys — spec/30-processes/config-store.md
 */

#ifndef CONFIG_KEYS_H
#define CONFIG_KEYS_H

#define CONFIG_GROUP_WIFI   "wifi"
#define CONFIG_KEY_WIFI_SSID "ssid"
#define CONFIG_KEY_WIFI_PASS "pass"

#define CONFIG_GROUP_MQTT          "mqtt"
#define CONFIG_KEY_MQTT_HOST       "host"
#define CONFIG_KEY_MQTT_PORT       "port"
#define CONFIG_KEY_MQTT_USER       "user"
#define CONFIG_KEY_MQTT_PASS       "pass"
#define CONFIG_KEY_MQTT_DEVICE_ID  "device_id"
#define CONFIG_KEY_MQTT_TLS        "tls"

#define CONFIG_GROUP_SYSTEM        "system"
#define CONFIG_KEY_BOOT_COUNT      "boot_count"
#define CONFIG_KEY_OTA_PENDING     "ota_pending"

#define CONFIG_GROUP_CALIB         "calib"
#define CONFIG_KEY_CALIB_ZERO      "zero"
#define CONFIG_KEY_CALIB_SPAN_G    "span_g"
#define CONFIG_KEY_CALIB_SPAN_RAW  "span_raw"

#endif /* CONFIG_KEYS_H */
