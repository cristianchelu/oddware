/*
 * Wi-Fi boot policy — spec/30-processes/wifi-lifecycle.md § Bank-B boot delay
 */

#ifndef WIFI_BOOT_POLICY_H
#define WIFI_BOOT_POLICY_H

#include <stdint.h>

#include "boot_bank.h"

uint32_t wifi_boot_connect_timeout_ms(boot_bank_t active_bank);

#endif /* WIFI_BOOT_POLICY_H */
