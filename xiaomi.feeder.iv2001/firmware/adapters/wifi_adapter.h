/*
 * Wi-Fi adapter helpers — spec/40-architecture/ports.md
 */

#ifndef WIFI_ADAPTER_H
#define WIFI_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#include "provision_scan_list.h"

void wifi_adapter_stack_init(void);
void wifi_adapter_clear_sdk_sta_profile(void);

size_t wifi_adapter_scan_networks(provision_scan_ap_t *out,
                                    size_t max_out,
                                    uint32_t timeout_ms);

#endif /* WIFI_ADAPTER_H */
