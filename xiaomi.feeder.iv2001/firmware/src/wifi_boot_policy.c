/*
 * Wi-Fi boot policy — spec/30-processes/wifi-lifecycle.md § Bank-B boot delay
 */

#include "wifi_boot_policy.h"
#include "wifi_session.h"

#define WIFI_BOOT_BANK_A_CONNECT_TIMEOUT_MS WIFI_SESSION_CONNECT_TIMEOUT_MS

/*
 * Bank-B boots: headroom for N9 ROM seek_and_connect idle (~30 s) before
 * PORT_SECURE. Bank A uses the same ceiling until bench proves a shorter
 * budget is safe. See spec/30-processes/wifi-lifecycle.md § Bank-B boot delay.
 */
#define WIFI_BOOT_BANK_B_CONNECT_TIMEOUT_MS WIFI_SESSION_CONNECT_TIMEOUT_MS

uint32_t wifi_boot_connect_timeout_ms(boot_bank_t active_bank)
{
    if (active_bank == BOOT_BANK_B) {
        return WIFI_BOOT_BANK_B_CONNECT_TIMEOUT_MS;
    }

    return WIFI_BOOT_BANK_A_CONNECT_TIMEOUT_MS;
}
