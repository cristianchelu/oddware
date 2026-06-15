/* Tests: spec/30-processes/wifi-lifecycle.md (SDK STA profile invalidate) */

#include <stdbool.h>
#include <string.h>

#include "unity.h"

#include "nvdm_record.h"
#include "wifi_sdk_profile.h"

static bool record_has(const char *group, const char *key, const uint8_t *data, uint32_t len)
{
    size_t i;

    for (i = 0u; i < nvdm_record_count(); i++) {
        const nvdm_record_entry_t *entry = nvdm_record_get(i);

        if (entry == NULL) {
            continue;
        }

        if (strcmp(entry->group, group) != 0 || strcmp(entry->key, key) != 0) {
            continue;
        }

        if (entry->len != len) {
            continue;
        }

        if (memcmp(entry->data, data, len) == 0) {
            return true;
        }
    }

    return false;
}

void test_invalidate_clears_sta_ssid_and_psk_keys(void)
{
    const char zero[] = "0";

    nvdm_record_reset();
    wifi_sdk_profile_invalidate();

    TEST_ASSERT_TRUE(record_has("STA", "SsidLen", (const uint8_t *)zero, 1u));
    TEST_ASSERT_TRUE(record_has("STA", "Ssid", (const uint8_t *)"", 0u));
    TEST_ASSERT_TRUE(record_has("STA", "WpaPskLen", (const uint8_t *)zero, 1u));
    TEST_ASSERT_TRUE(record_has("STA", "WpaPsk", (const uint8_t *)"", 0u));
    TEST_ASSERT_EQUAL_size_t(4u, nvdm_record_count());
}
