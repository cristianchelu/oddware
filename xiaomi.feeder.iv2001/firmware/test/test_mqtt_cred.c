/* Tests: spec/30-processes/uart-console.md (MQTT broker rules) */

#include <string.h>

#include "unity.h"
#include "mqtt_cred.h"
#include "config_keys.h"
#include "fake_config_port.h"

void test_valid_host_passes_validation(void)
{
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_validate_host("mqtt.local"));
}

void test_empty_host_rejected(void)
{
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, mqtt_cred_validate_host(""));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, mqtt_cred_validate_host(NULL));
}

void test_port_out_of_range_rejected(void)
{
    const config_port_t *cfg = fake_config_port_get();
    mqtt_cred_t cred;

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, mqtt_cred_validate_port(0));

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, "broker"));
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PORT, "65536"));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, mqtt_cred_load(cfg, &cred));
}

void test_device_id_allows_alphanumeric_and_underscore(void)
{
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_validate_device_id("feeder_01"));
}

void test_device_id_rejects_invalid_chars(void)
{
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, mqtt_cred_validate_device_id("bad id"));
}

void test_mqtt_save_and_load_round_trip(void)
{
    const config_port_t *cfg = fake_config_port_get();
    mqtt_cred_t cred;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_host(cfg, "broker.local"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_port(cfg, 1883));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_user(cfg, "user1"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_pass(cfg, "secret"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_device_id(cfg, "a4cf12"));

    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_load(cfg, &cred));
    TEST_ASSERT_EQUAL_STRING("broker.local", cred.host);
    TEST_ASSERT_EQUAL_UINT16(1883, cred.port);
    TEST_ASSERT_EQUAL_STRING("user1", cred.user);
    TEST_ASSERT_EQUAL_STRING("secret", cred.pass);
    TEST_ASSERT_EQUAL_STRING("a4cf12", cred.device_id);
    TEST_ASSERT_FALSE(cred.tls);
}

void test_load_defaults_port_when_missing(void)
{
    const config_port_t *cfg = fake_config_port_get();
    mqtt_cred_t cred;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, "broker"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_load(cfg, &cred));
    TEST_ASSERT_EQUAL_UINT16(1883, cred.port);
}

void test_load_missing_host_returns_not_found(void)
{
    const config_port_t *cfg = fake_config_port_get();
    mqtt_cred_t cred;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_FOUND, mqtt_cred_load(cfg, &cred));
}

void test_is_stored_false_when_host_missing(void)
{
    fake_config_port_reset();
    TEST_ASSERT_FALSE(mqtt_cred_is_stored(fake_config_port_get()));
}

void test_mqtt_is_stored_true_after_save(void)
{
    fake_config_port_reset();
    mqtt_cred_save_host(fake_config_port_get(), "broker");
    TEST_ASSERT_TRUE(mqtt_cred_is_stored(fake_config_port_get()));
}

void test_resolve_device_id_uses_nvdm_value(void)
{
    mqtt_cred_t cred;
    char resolved[16];

    memset(&cred, 0, sizeof(cred));
    strcpy(cred.device_id, "custom");
    mqtt_cred_resolve_device_id(&cred, "aabbccddeeff", resolved, sizeof(resolved));
    TEST_ASSERT_EQUAL_STRING("custom", resolved);
}

void test_resolve_device_id_falls_back_to_mac(void)
{
    mqtt_cred_t cred;
    char resolved[16];

    memset(&cred, 0, sizeof(cred));
    mqtt_cred_resolve_device_id(&cred, "aabbccddeeff", resolved, sizeof(resolved));
    TEST_ASSERT_EQUAL_STRING("ddeeff", resolved);
}

void test_load_tls_enabled_returns_not_supported(void)
{
    const config_port_t *cfg = fake_config_port_get();
    mqtt_cred_t cred;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, "broker"));
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_TLS, "true"));
    TEST_ASSERT_EQUAL(PORT_ERR_NOT_SUPPORTED, mqtt_cred_load(cfg, &cred));
}

void test_port_boundaries_valid(void)
{
    const config_port_t *cfg = fake_config_port_get();
    mqtt_cred_t cred;

    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_validate_port(1));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_validate_port(65535));

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, "broker"));
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PORT, "1"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_load(cfg, &cred));
    TEST_ASSERT_EQUAL_UINT16(1, cred.port);

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, "broker"));
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PORT, "65535"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_load(cfg, &cred));
    TEST_ASSERT_EQUAL_UINT16(65535, cred.port);
}

void test_load_rejects_port_zero_in_nvdm(void)
{
    const config_port_t *cfg = fake_config_port_get();
    mqtt_cred_t cred;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, "broker"));
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PORT, "0"));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, mqtt_cred_load(cfg, &cred));
}

void test_anonymous_credentials_load(void)
{
    const config_port_t *cfg = fake_config_port_get();
    mqtt_cred_t cred;

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_host(cfg, "broker.local"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_user(cfg, ""));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_pass(cfg, ""));

    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_load(cfg, &cred));
    TEST_ASSERT_EQUAL_STRING("", cred.user);
    TEST_ASSERT_EQUAL_STRING("", cred.pass);
}

void test_device_id_hyphen_allowed(void)
{
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_validate_device_id("feeder-01"));
}

void test_device_id_max_length_enforced(void)
{
    char id32[33];
    char id33[34];

    memset(id32, 'a', 32);
    id32[32] = '\0';
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_validate_device_id(id32));

    memset(id33, 'a', 33);
    id33[33] = '\0';
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, mqtt_cred_validate_device_id(id33));
}

void test_is_stored_false_when_host_empty_string(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, cfg->write(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, ""));
    TEST_ASSERT_FALSE(mqtt_cred_is_stored(cfg));
}
