/* Tests: spec/30-processes/provisioning-flow.md (Wi-Fi scan list) */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "provision_scan_list.h"

void test_merge_dedupes_and_sorts_by_rssi(void)
{
    provision_scan_list_t list;
    provision_scan_ap_t raw[] = {
        { "WeakNet", -70 },
        { "HomeNet", -45 },
        { "HomeNet", -55 },
        { "OpenNet", -60 },
    };

    provision_scan_list_clear(&list);
    TEST_ASSERT_EQUAL(3, provision_scan_list_merge(&list, raw, 4, NULL));
    TEST_ASSERT_EQUAL_STRING("HomeNet", list.aps[0].ssid);
    TEST_ASSERT_EQUAL(-45, list.aps[0].rssi);
    TEST_ASSERT_EQUAL_STRING("OpenNet", list.aps[1].ssid);
    TEST_ASSERT_EQUAL_STRING("WeakNet", list.aps[2].ssid);
}

void test_merge_excludes_provisioning_ap(void)
{
    provision_scan_list_t list;
    provision_scan_ap_t raw[] = {
        { "PetFeeder-8722", -30 },
        { "HomeNet", -50 },
    };

    provision_scan_list_clear(&list);
    TEST_ASSERT_EQUAL(1, provision_scan_list_merge(&list, raw, 2, "PetFeeder-8722"));
    TEST_ASSERT_EQUAL_STRING("HomeNet", list.aps[0].ssid);
}

void test_merge_skips_empty_ssid(void)
{
    provision_scan_list_t list;
    provision_scan_ap_t raw[] = {
        { "", -40 },
        { "HomeNet", -50 },
    };

    provision_scan_list_clear(&list);
    TEST_ASSERT_EQUAL(1, provision_scan_list_merge(&list, raw, 2, NULL));
    TEST_ASSERT_EQUAL_STRING("HomeNet", list.aps[0].ssid);
}

void test_merge_null_args_returns_zero(void)
{
    provision_scan_list_t list;
    provision_scan_ap_t raw[] = { { "HomeNet", -50 } };

    provision_scan_list_clear(&list);
    TEST_ASSERT_EQUAL(0, provision_scan_list_merge(NULL, raw, 1, NULL));
    TEST_ASSERT_EQUAL(0, provision_scan_list_merge(&list, NULL, 1, NULL));
}

void test_merge_empty_input_is_noop(void)
{
    provision_scan_list_t list;

    provision_scan_list_clear(&list);
    TEST_ASSERT_EQUAL(0, provision_scan_list_merge(&list, NULL, 0, NULL));
}

void test_merge_upgrades_rssi_and_reorders(void)
{
    provision_scan_list_t list;
    provision_scan_ap_t first[] = { { "HomeNet", -60 }, { "OpenNet", -50 } };
    provision_scan_ap_t better[] = { { "HomeNet", -40 } };

    provision_scan_list_clear(&list);
    provision_scan_list_merge(&list, first, 2, NULL);
    provision_scan_list_merge(&list, better, 1, NULL);
    TEST_ASSERT_EQUAL(2, list.count);
    TEST_ASSERT_EQUAL_STRING("HomeNet", list.aps[0].ssid);
    TEST_ASSERT_EQUAL(-40, list.aps[0].rssi);
    TEST_ASSERT_EQUAL_STRING("OpenNet", list.aps[1].ssid);
}

void test_merge_caps_at_max_and_keeps_strongest(void)
{
    provision_scan_list_t list;
    provision_scan_ap_t raw[PROVISION_SCAN_MAX_APS + 2];
    size_t i;

    provision_scan_list_clear(&list);
    for (i = 0; i < PROVISION_SCAN_MAX_APS + 2; i++) {
        snprintf(raw[i].ssid, sizeof(raw[i].ssid), "Net%02u", (unsigned)i);
        raw[i].rssi = (int8_t)(-90 + (int)i);
    }

    TEST_ASSERT_EQUAL(PROVISION_SCAN_MAX_APS,
                      provision_scan_list_merge(&list, raw, PROVISION_SCAN_MAX_APS + 2, NULL));
    TEST_ASSERT_EQUAL_STRING("Net17", list.aps[0].ssid);
    TEST_ASSERT_EQUAL(-73, list.aps[0].rssi);
    TEST_ASSERT_EQUAL_STRING("Net02", list.aps[PROVISION_SCAN_MAX_APS - 1].ssid);
}

void test_merge_ignores_weaker_duplicate(void)
{
    provision_scan_list_t list;
    provision_scan_ap_t raw[] = { { "HomeNet", -45 }, { "HomeNet", -80 } };

    provision_scan_list_clear(&list);
    TEST_ASSERT_EQUAL(1, provision_scan_list_merge(&list, raw, 2, NULL));
    TEST_ASSERT_EQUAL(-45, list.aps[0].rssi);
}
