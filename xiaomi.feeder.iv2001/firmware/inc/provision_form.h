/*
 * Captive-portal form parsing and HTML — spec/30-processes/provisioning-flow.md
 */

#ifndef PROVISION_FORM_H
#define PROVISION_FORM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "mqtt_cred.h"
#include "port_err.h"
#include "provision_scan_list.h"
#include "wifi_cred.h"

#define PROVISION_MSG_VALIDATION   "Please fix the highlighted fields."
#define PROVISION_MSG_WIFI_FAIL    "Connection failed"
#define PROVISION_MSG_WIFI_FAIL_PREFIX "Could not connect to \""
#define PROVISION_MSG_WIFI_FAIL_SUFFIX "\". Check the network name and password, then try again."
#define PROVISION_MSG_MQTT_FAIL    "Could not connect to the MQTT broker. Check the host, port, and credentials, then try again."
#define PROVISION_MSG_SAVE_FAIL    "Could not save configuration. Try again."
#define PROVISION_MSG_SUCCESS      "Setup complete. Rebooting in 3 seconds…"

#define PROVISION_FIELD_WIFI_SSID      "wifi_ssid"
#define PROVISION_FIELD_WIFI_SSID_PICK "wifi_ssid_pick"
#define PROVISION_FIELD_WIFI_PASS      "wifi_pass"
#define PROVISION_FIELD_MQTT_HOST   "mqtt_host"
#define PROVISION_FIELD_MQTT_PORT   "mqtt_port"
#define PROVISION_FIELD_MQTT_USER   "mqtt_user"
#define PROVISION_FIELD_MQTT_PASS   "mqtt_pass"
#define PROVISION_FIELD_DEVICE_ID   "device_id"

typedef struct provision_input {
    char wifi_ssid[WIFI_SSID_MAX_LEN + 1];
    char wifi_pass[WIFI_PASS_MAX_LEN + 1];
    char mqtt_host[MQTT_HOST_MAX_LEN + 1];
    uint16_t mqtt_port;
    bool mqtt_port_set;
    char mqtt_user[MQTT_USER_MAX_LEN + 1];
    char mqtt_pass[MQTT_PASS_MAX_LEN + 1];
    char device_id[MQTT_DEVICE_ID_MAX_LEN + 1];
} provision_input_t;

void provision_input_init(provision_input_t *input);

port_err_t provision_form_parse_urlencoded(const char *body,
                                           size_t body_len,
                                           provision_input_t *out);

port_err_t provision_form_validate(const provision_input_t *input);

size_t provision_form_render(const provision_input_t *input,
                             const char *message,
                             const provision_scan_list_t *scan,
                             char *buf,
                             size_t len);

size_t provision_form_render_success(char *buf, size_t len);

size_t provision_form_wifi_fail_message(const char *ssid, char *buf, size_t len);

#endif /* PROVISION_FORM_H */
