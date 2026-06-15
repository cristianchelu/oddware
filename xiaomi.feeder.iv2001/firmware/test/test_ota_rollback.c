/* Tests: spec/30-processes/ota-flow.md (Rollback) */

#include <stdbool.h>

#include "unity.h"

#include "config_keys.h"
#include "fake_config_port.h"
#include "fake_time.h"
#include "ota_rollback.h"
#include "ota_rollback_host_stubs.h"

#define OTA_ROLLBACK_TIMEOUT_MS 120000u

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

void test_on_boot_no_pending_skips_timer(void)
{
    fake_config_port_reset();
    fake_time_reset();
    ota_rollback_host_stub_reset();

    ota_rollback_on_boot();

    TEST_ASSERT_EQUAL_UINT32(0, ota_rollback_poll_ms());
    TEST_ASSERT_EQUAL_size_t(0, ota_rollback_host_stub_bank_switch_calls());
}

void test_on_boot_pending_increments_boot_count_and_starts_timer(void)
{
    char buf[16];

    fake_config_port_reset();
    fake_time_reset();
    ota_rollback_host_stub_reset();
    system_config_set(CONFIG_KEY_OTA_PENDING, "1");

    ota_rollback_on_boot();

    TEST_ASSERT_TRUE(system_config_get(CONFIG_KEY_BOOT_COUNT, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("1", buf);
    TEST_ASSERT_EQUAL_UINT32(1000, ota_rollback_poll_ms());
    TEST_ASSERT_EQUAL_size_t(0, ota_rollback_host_stub_bank_switch_calls());
}

void test_mqtt_connected_clears_pending_and_boot_count(void)
{
    char buf[16];

    fake_config_port_reset();
    fake_time_reset();
    ota_rollback_host_stub_reset();
    system_config_set(CONFIG_KEY_OTA_PENDING, "1");
    system_config_set(CONFIG_KEY_BOOT_COUNT, "2");

    ota_rollback_on_boot();
    ota_rollback_on_mqtt_connected();

    TEST_ASSERT_TRUE(system_config_get(CONFIG_KEY_OTA_PENDING, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("0", buf);
    TEST_ASSERT_TRUE(system_config_get(CONFIG_KEY_BOOT_COUNT, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("0", buf);
    TEST_ASSERT_EQUAL_UINT32(0, ota_rollback_poll_ms());
    TEST_ASSERT_EQUAL_size_t(0, ota_rollback_host_stub_bank_switch_calls());
}

void test_poll_after_timeout_triggers_bank_switch(void)
{
    char buf[16];

    fake_config_port_reset();
    fake_time_reset();
    ota_rollback_host_stub_reset();
    system_config_set(CONFIG_KEY_OTA_PENDING, "1");

    ota_rollback_on_boot();
    fake_time_advance_ms(OTA_ROLLBACK_TIMEOUT_MS);

    TEST_ASSERT_EQUAL_UINT32(0, ota_rollback_poll_ms());
    TEST_ASSERT_EQUAL_size_t(1, ota_rollback_host_stub_bank_switch_calls());
    TEST_ASSERT_EQUAL_size_t(1, ota_rollback_host_stub_reboot_calls());
    TEST_ASSERT_TRUE(system_config_get(CONFIG_KEY_OTA_PENDING, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("0", buf);
}
