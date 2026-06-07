/*
 * OUI symbols required by prebuilt libwifi.a when not provided by the link set.
 * Declared in middleware/MTK/wifi_service/combo/inc/wifi_scan.h.
 */

#include <stdint.h>

#include "wifi_scan.h"

const unsigned char WPA_OUI[] = { 0x00, 0x50, 0xF2 };
const unsigned char RSN_OUI[] = { 0x00, 0x0F, 0xAC };
const unsigned char WPS_OUI[] = { 0x00, 0x50, 0xF2, 0x04 };
uint8_t ZeroSsid[32];
