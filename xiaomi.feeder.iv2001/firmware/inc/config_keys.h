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

#define CONFIG_GROUP_CALIB         "calib"
#define CONFIG_KEY_CALIB_ZERO      "zero"
#define CONFIG_KEY_CALIB_SPAN_G    "span_g"
#define CONFIG_KEY_CALIB_SPAN_RAW  "span_raw"

#define CONFIG_GROUP_FEED               "feed"
#define CONFIG_KEY_FEED_MODE            "mode"
#define CONFIG_KEY_CHILD_LOCK           "child_lock"
#define CONFIG_KEY_OVERFILL_ENABLED     "overfill_enabled"
#define CONFIG_KEY_OVERFILL_THRESHOLD_G "overfill_threshold_g"

#define CONFIG_GROUP_POWER                 "power"
#define CONFIG_KEY_POWER_BATT_SCALE_X1000  "batt_scale_x1000"

#define CONFIG_GROUP_TIME          "time"
#define CONFIG_KEY_TZ_RULE         "tz_rule"
#define CONFIG_KEY_TZ_LABEL        "tz_label"

#define CONFIG_GROUP_SCHEDULE      "schedule"
#define CONFIG_KEY_SCHEDULE_ENABLED "enabled"
#define CONFIG_KEY_SCHEDULE_SLOTS  "slots"
#define CONFIG_KEY_SCHEDULE_RUNTIME "runtime"  /* IF1R v1 — future persistence */

#endif /* CONFIG_KEYS_H */
