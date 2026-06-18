/* Tests: spec/30-processes/ota-flow.md (Slot health) */

#include <stdbool.h>

#include "unity.h"

#include "config_keys.h"
#include "boot_bank_target.h"
#include "fake_boot_bank.h"
#include "fake_config_port.h"
#include "fake_time.h"
#include "ota_slot_health.h"

#define OTA_SLOT_CONFIRM_MS 60000u

static void system_config_set(const char *key, const char *value)
{
    const config_port_t *cfg = fake_config_port_get();

    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_SYSTEM, key, value));
}

static bool system_config_get(const char *key, char *buf, size_t len)
{
    const config_port_t *cfg = fake_config_port_get();

    return cfg->read(CONFIG_GROUP_SYSTEM, key, buf, len) == PORT_OK;
}

void test_on_boot_verified_skips_timer(void)
{
    fake_config_port_reset();
    fake_time_reset();
    fake_boot_bank_reset();
    fake_boot_bank_set_unverified(false);

    ota_slot_health_on_boot();

    TEST_ASSERT_EQUAL_UINT32(0, ota_slot_health_poll_ms());
    TEST_ASSERT_EQUAL_size_t(0, fake_boot_bank_confirm_calls());
}

void test_on_boot_unverified_increments_boot_count_and_starts_timer(void)
{
    char buf[16];

    fake_config_port_reset();
    fake_time_reset();
    fake_boot_bank_reset();
    fake_boot_bank_set_unverified(true);

    ota_slot_health_on_boot();

    TEST_ASSERT_TRUE(system_config_get(CONFIG_KEY_BOOT_COUNT, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("1", buf);
    TEST_ASSERT_EQUAL_UINT32(1000, ota_slot_health_poll_ms());
    TEST_ASSERT_EQUAL_size_t(0, fake_boot_bank_confirm_calls());
}

void test_poll_after_confirm_window_clears_slot_health(void)
{
    char buf[16];

    fake_config_port_reset();
    fake_time_reset();
    fake_boot_bank_reset();
    fake_boot_bank_set_unverified(true);
    system_config_set(CONFIG_KEY_BOOT_COUNT, "2");

    ota_slot_health_on_boot();
    fake_time_advance_ms(OTA_SLOT_CONFIRM_MS);

    TEST_ASSERT_EQUAL_UINT32(0, ota_slot_health_poll_ms());
    TEST_ASSERT_EQUAL_size_t(1, fake_boot_bank_confirm_calls());
    TEST_ASSERT_FALSE(boot_bank_query_unverified());
    TEST_ASSERT_TRUE(system_config_get(CONFIG_KEY_BOOT_COUNT, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("0", buf);
}

void test_poll_before_confirm_window_keeps_timer(void)
{
    fake_config_port_reset();
    fake_time_reset();
    fake_boot_bank_reset();
    fake_boot_bank_set_unverified(true);

    ota_slot_health_on_boot();
    fake_time_advance_ms(OTA_SLOT_CONFIRM_MS - 1u);

    TEST_ASSERT_EQUAL_UINT32(1000, ota_slot_health_poll_ms());
    TEST_ASSERT_EQUAL_size_t(0, fake_boot_bank_confirm_calls());
    TEST_ASSERT_TRUE(boot_bank_query_unverified());
}
