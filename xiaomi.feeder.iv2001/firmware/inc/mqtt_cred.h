/*
 * MQTT broker config validation and NVDM storage.
 * spec/30-processes/uart-console.md (MQTT broker rules)
 */

#ifndef MQTT_CRED_H
#define MQTT_CRED_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "config_port.h"
#include "port_err.h"

#define MQTT_HOST_MAX_LEN      253
#define MQTT_USER_MAX_LEN      64
#define MQTT_PASS_MAX_LEN      64
#define MQTT_DEVICE_ID_MAX_LEN 32

typedef struct mqtt_cred {
    char host[MQTT_HOST_MAX_LEN + 1];
    uint16_t port;
    char user[MQTT_USER_MAX_LEN + 1];
    char pass[MQTT_PASS_MAX_LEN + 1];
    char device_id[MQTT_DEVICE_ID_MAX_LEN + 1];
    bool tls;
} mqtt_cred_t;

port_err_t mqtt_cred_validate_host(const char *host);
port_err_t mqtt_cred_validate_port(uint16_t port);
port_err_t mqtt_cred_validate_device_id(const char *device_id);

port_err_t mqtt_cred_save_host(const config_port_t *cfg, const char *host);
port_err_t mqtt_cred_save_port(const config_port_t *cfg, uint16_t port);
port_err_t mqtt_cred_save_user(const config_port_t *cfg, const char *user);
port_err_t mqtt_cred_save_pass(const config_port_t *cfg, const char *pass);
port_err_t mqtt_cred_save_device_id(const config_port_t *cfg, const char *device_id);

port_err_t mqtt_cred_load(const config_port_t *cfg, mqtt_cred_t *out);

bool mqtt_cred_is_stored(const config_port_t *cfg);

void mqtt_cred_resolve_device_id(const mqtt_cred_t *cred,
                                 const char *mac_hex12,
                                 char *buf,
                                 size_t len);

#endif /* MQTT_CRED_H */
