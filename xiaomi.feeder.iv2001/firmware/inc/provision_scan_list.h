/*
 * Provisioning Wi-Fi scan results — spec/30-processes/provisioning-flow.md
 */

#ifndef PROVISION_SCAN_LIST_H
#define PROVISION_SCAN_LIST_H

#include <stddef.h>
#include <stdint.h>

#include "wifi_cred.h"

#define PROVISION_SCAN_MAX_APS 16

typedef struct {
    char ssid[WIFI_SSID_MAX_LEN + 1];
    int8_t rssi;
} provision_scan_ap_t;

typedef struct {
    provision_scan_ap_t aps[PROVISION_SCAN_MAX_APS];
    size_t count;
} provision_scan_list_t;

void provision_scan_list_clear(provision_scan_list_t *list);

size_t provision_scan_list_merge(provision_scan_list_t *list,
                                 const provision_scan_ap_t *raw,
                                 size_t raw_count,
                                 const char *exclude_ssid);

#endif /* PROVISION_SCAN_LIST_H */
