/* Tests: spec/30-processes/provisioning-flow.md (stashed portal error) */

#include <string.h>

#include "unity.h"

#include "provision_pending.h"

void test_pending_take_returns_false_when_empty(void)
{
    provision_input_t input;
    char msg[PROVISION_PENDING_MSG_MAX];

    provision_pending_clear();
    TEST_ASSERT_FALSE(provision_pending_take(&input, msg, sizeof(msg)));
}

void test_pending_peek_does_not_clear(void)
{
    provision_input_t stored;
    provision_input_t peeked;
    char msg[PROVISION_PENDING_MSG_MAX];

    provision_pending_clear();
    provision_input_init(&stored);
    strcpy(stored.wifi_ssid, "HomeNet");
    provision_pending_set(&stored, PROVISION_MSG_MQTT_FAIL);

    TEST_ASSERT_TRUE(provision_pending_peek(&peeked, msg, sizeof(msg)));
    TEST_ASSERT_EQUAL_STRING("HomeNet", peeked.wifi_ssid);
    TEST_ASSERT_TRUE(provision_pending_peek(&peeked, msg, sizeof(msg)));
    TEST_ASSERT_TRUE(provision_pending_take(&peeked, msg, sizeof(msg)));
    TEST_ASSERT_FALSE(provision_pending_peek(&peeked, msg, sizeof(msg)));
}

void test_pending_set_and_take_round_trip(void)
{
    provision_input_t stored;
    provision_input_t taken;
    char msg[PROVISION_PENDING_MSG_MAX];

    provision_pending_clear();
    provision_input_init(&stored);
    strcpy(stored.wifi_ssid, "HomeNet");
    strcpy(stored.wifi_pass, "password1");
    strcpy(stored.mqtt_host, "broker.local");

    provision_pending_set(&stored, PROVISION_MSG_MQTT_FAIL);
    TEST_ASSERT_TRUE(provision_pending_take(&taken, msg, sizeof(msg)));
    TEST_ASSERT_EQUAL_STRING("HomeNet", taken.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("password1", taken.wifi_pass);
    TEST_ASSERT_EQUAL_STRING("broker.local", taken.mqtt_host);
    TEST_ASSERT_EQUAL_STRING(PROVISION_MSG_MQTT_FAIL, msg);
    TEST_ASSERT_FALSE(provision_pending_take(&taken, msg, sizeof(msg)));
}

void test_pending_clear_discards_state(void)
{
    provision_input_t input;
    char msg[PROVISION_PENDING_MSG_MAX];

    provision_pending_clear();
    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    provision_pending_set(&input, PROVISION_MSG_WIFI_FAIL);
    provision_pending_clear();
    TEST_ASSERT_FALSE(provision_pending_take(&input, msg, sizeof(msg)));
}
