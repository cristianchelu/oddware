/* Tests: spec/30-processes/uart-console.md (config factory-reset) */

#include "unity.h"

#include "fake_config_port.h"
#include "mqtt_cred.h"
#include "provision_reset.h"
#include "wifi_cred.h"

void test_erase_app_groups_succeeds_when_optional_namespaces_missing(void)
{
    fake_config_port_reset();
    wifi_cred_save(fake_config_port_get(), "HomeNet", "password1");
    mqtt_cred_save_host(fake_config_port_get(), "broker.local");

    TEST_ASSERT_TRUE(provision_erase_app_groups(fake_config_port_get()));
    TEST_ASSERT_FALSE(wifi_cred_is_stored(fake_config_port_get()));
    TEST_ASSERT_FALSE(mqtt_cred_is_stored(fake_config_port_get()));
}

void test_erase_app_groups_fails_on_io_error(void)
{
    (void)0;
    TEST_IGNORE_MESSAGE("no injectable IO fault in fake_config_port yet");
}
