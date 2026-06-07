/* Tests: spec/30-processes/uart-console.md (Wi-Fi credential rules) */

#include <string.h>

#include "unity.h"
#include "wifi_cred.h"
#include "config_keys.h"
#include "fake_config_port.h"

void test_valid_credentials_pass_validation(void)
{
    TEST_ASSERT_EQUAL(PORT_OK, wifi_cred_validate("HomeNet", "secretpass"));
}

void test_empty_ssid_rejected(void)
{
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, wifi_cred_validate("", "secretpass"));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, wifi_cred_validate(NULL, "secretpass"));
}

void test_ssid_too_long_rejected(void)
{
    char ssid[WIFI_SSID_MAX_LEN + 2];

    memset(ssid, 'a', sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, wifi_cred_validate(ssid, "secretpass"));
}

void test_password_too_long_rejected(void)
{
    char pass[WIFI_PASS_MAX_LEN + 2];

    memset(pass, 'p', sizeof(pass) - 1);
    pass[sizeof(pass) - 1] = '\0';
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, wifi_cred_validate("HomeNet", pass));
}

void test_short_password_rejected(void)
{
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, wifi_cred_validate("HomeNet", "short"));
}

void test_open_network_empty_password_allowed(void)
{
    TEST_ASSERT_EQUAL(PORT_OK, wifi_cred_validate("OpenNet", ""));
}

void test_save_and_load_round_trip(void)
{
    const config_port_t *cfg = fake_config_port_get();
    char ssid[64];
    char pass[64];

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, wifi_cred_save(cfg, "MySSID", "mypassword"));
    TEST_ASSERT_EQUAL(PORT_OK, wifi_cred_load(cfg, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("MySSID", ssid);
    TEST_ASSERT_EQUAL_STRING("mypassword", pass);
}

void test_load_missing_credentials_returns_not_found(void)
{
    const config_port_t *cfg = fake_config_port_get();
    char ssid[64];
    char pass[64];

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_FOUND,
                      wifi_cred_load(cfg, ssid, sizeof(ssid), pass, sizeof(pass)));
}

void test_is_stored_false_when_ssid_missing(void)
{
    fake_config_port_reset();
    TEST_ASSERT_FALSE(wifi_cred_is_stored(fake_config_port_get()));
}

void test_is_stored_true_after_save(void)
{
    fake_config_port_reset();
    wifi_cred_save(fake_config_port_get(), "Net", "password1");
    TEST_ASSERT_TRUE(wifi_cred_is_stored(fake_config_port_get()));
}

void test_save_rejects_invalid_credentials(void)
{
    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      wifi_cred_save(fake_config_port_get(), "", "password1"));
    TEST_ASSERT_FALSE(wifi_cred_is_stored(fake_config_port_get()));
}

void test_load_ssid_only_treats_missing_pass_as_open(void)
{
    const config_port_t *cfg = fake_config_port_get();
    char ssid[64];
    char pass[64];

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_SSID, "OpenNet"));
    TEST_ASSERT_EQUAL(PORT_OK, wifi_cred_load(cfg, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("OpenNet", ssid);
    TEST_ASSERT_EQUAL_STRING("", pass);
    TEST_ASSERT_TRUE(wifi_cred_is_open_network(pass));
}

void test_load_explicit_empty_pass_is_open_network(void)
{
    const config_port_t *cfg = fake_config_port_get();
    char ssid[64];
    char pass[64];

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, wifi_cred_save(cfg, "OpenNet", ""));
    TEST_ASSERT_EQUAL(PORT_OK, wifi_cred_load(cfg, ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_TRUE(wifi_cred_is_open_network(pass));
}

void test_is_stored_true_with_ssid_only(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_SSID, "OpenNet"));
    TEST_ASSERT_TRUE(wifi_cred_is_stored(cfg));
}
