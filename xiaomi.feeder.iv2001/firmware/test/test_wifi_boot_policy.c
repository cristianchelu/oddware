/* Tests: spec/30-processes/wifi-lifecycle.md § Bank-B boot delay */

#include "unity.h"

#include "boot_bank.h"
#include "wifi_boot_policy.h"

void test_bank_a_connect_timeout_default(void)
{
    TEST_ASSERT_EQUAL_UINT32(60000u, wifi_boot_connect_timeout_ms(BOOT_BANK_A));
}

void test_bank_b_connect_timeout_allows_n9_idle_gap(void)
{
    TEST_ASSERT_EQUAL_UINT32(60000u, wifi_boot_connect_timeout_ms(BOOT_BANK_B));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(35000u,
                                         wifi_boot_connect_timeout_ms(BOOT_BANK_B));
}
