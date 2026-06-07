/*
 * Wi-Fi credential validation and NVDM storage.
 * spec/30-processes/uart-console.md (Wi-Fi credential rules)
 */

#ifndef WIFI_CRED_H
#define WIFI_CRED_H

#include <stddef.h>
#include <stdbool.h>

#include "config_port.h"
#include "port_err.h"

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 63
#define WIFI_PASS_MIN_LEN 8

port_err_t wifi_cred_validate(const char *ssid, const char *pass);

port_err_t wifi_cred_save(const config_port_t *cfg, const char *ssid, const char *pass);

port_err_t wifi_cred_load(const config_port_t *cfg,
                          char *ssid, size_t ssid_len,
                          char *pass, size_t pass_len);

bool wifi_cred_is_stored(const config_port_t *cfg);

bool wifi_cred_is_open_network(const char *pass);

#endif /* WIFI_CRED_H */
