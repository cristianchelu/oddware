/* Tests: spec/30-processes/provisioning-flow.md (Save + connect flow) */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "mqtt_cred.h"
#include "provision_flow.h"
#include "wifi_cred.h"

static bool s_wifi_try_ok = true;
static bool s_mqtt_try_ok = true;
static int s_wifi_try_calls;
static int s_mqtt_try_calls;

static bool test_wifi_try_connect(const char *ssid, const char *pass, uint32_t timeout_ms)
{
    (void)ssid;
    (void)pass;
    (void)timeout_ms;
    s_wifi_try_calls++;
    return s_wifi_try_ok;
}

static bool test_mqtt_try_connect(const provision_input_t *input, uint32_t timeout_ms)
{
    (void)input;
    (void)timeout_ms;
    s_mqtt_try_calls++;
    return s_mqtt_try_ok;
}

static const provision_flow_ops_t s_ops = {
    .save_wifi = NULL,
    .save_mqtt = NULL,
    .wifi_try_connect = test_wifi_try_connect,
    .mqtt_try_connect = test_mqtt_try_connect,
};

static void provision_flow_test_reset(void)
{
    fake_config_port_reset();
    s_wifi_try_ok = true;
    s_mqtt_try_ok = true;
    s_wifi_try_calls = 0;
    s_mqtt_try_calls = 0;
}

void test_submit_success_saves_wifi_and_mqtt(void)
{
    provision_input_t input;
    provision_flow_result_t result;
    char ssid[33];
    char pass[64];
    mqtt_cred_t mqtt;

    provision_flow_test_reset();
    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    strcpy(input.wifi_pass, "password1");
    strcpy(input.mqtt_host, "broker.local");
    input.mqtt_port_set = true;
    input.mqtt_port = 1883;

    result = provision_flow_submit(&input, fake_config_port_get(), &s_ops);
    TEST_ASSERT_EQUAL(PROVISION_FLOW_OK, result);
    TEST_ASSERT_EQUAL(1, s_wifi_try_calls);
    TEST_ASSERT_EQUAL(1, s_mqtt_try_calls);

    TEST_ASSERT_EQUAL(PORT_OK, wifi_cred_load(fake_config_port_get(), ssid, sizeof(ssid), pass, sizeof(pass)));
    TEST_ASSERT_EQUAL_STRING("HomeNet", ssid);
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_load(fake_config_port_get(), &mqtt));
    TEST_ASSERT_EQUAL_STRING("broker.local", mqtt.host);
}

void test_submit_wifi_failure_does_not_save(void)
{
    provision_input_t input;

    provision_flow_test_reset();
    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    strcpy(input.wifi_pass, "password1");
    strcpy(input.mqtt_host, "broker.local");

    s_wifi_try_ok = false;
    TEST_ASSERT_EQUAL(PROVISION_FLOW_WIFI_FAIL,
                      provision_flow_submit(&input, fake_config_port_get(), &s_ops));
    TEST_ASSERT_FALSE(wifi_cred_is_stored(fake_config_port_get()));
}

void test_submit_mqtt_failure_still_saves_wifi(void)
{
    provision_input_t input;
    char ssid[33];
    char pass[64];

    provision_flow_test_reset();
    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    strcpy(input.wifi_pass, "password1");
    strcpy(input.mqtt_host, "broker.local");

    s_mqtt_try_ok = false;
    TEST_ASSERT_EQUAL(PROVISION_FLOW_MQTT_WARN,
                      provision_flow_submit(&input, fake_config_port_get(), &s_ops));
    TEST_ASSERT_EQUAL_STRING(PROVISION_MSG_MQTT_WARN,
                             provision_flow_message(PROVISION_FLOW_MQTT_WARN));
    TEST_ASSERT_EQUAL(PORT_OK, wifi_cred_load(fake_config_port_get(), ssid, sizeof(ssid), pass, sizeof(pass)));
}

void test_submit_validation_failure(void)
{
    provision_input_t input;

    provision_flow_test_reset();
    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    strcpy(input.wifi_pass, "short");
    strcpy(input.mqtt_host, "broker.local");

    TEST_ASSERT_EQUAL(PROVISION_FLOW_VALIDATION_FAIL,
                      provision_flow_submit(&input, fake_config_port_get(), &s_ops));
    TEST_ASSERT_EQUAL(0, s_wifi_try_calls);
}
