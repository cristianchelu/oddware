/* Tests: spec/30-processes/mqtt-protocol.md (Topic namespace) */

#include <string.h>

#include "unity.h"
#include "mqtt_topics.h"

void test_topic_format_state_path(void)
{
    char topic[64];

    TEST_ASSERT_EQUAL(PORT_OK, mqtt_topic_format(topic, sizeof(topic), "a4cf12", "state"));
    TEST_ASSERT_EQUAL_STRING("petfeeder/a4cf12/state", topic);
}

void test_topic_format_rejects_empty_device_id(void)
{
    char topic[64];

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, mqtt_topic_format(topic, sizeof(topic), "", "state"));
}

void test_client_id_format(void)
{
    char client_id[64];

    TEST_ASSERT_EQUAL(PORT_OK, mqtt_client_id_format(client_id, sizeof(client_id), "a4cf12"));
    TEST_ASSERT_EQUAL_STRING("petfeeder_a4cf12", client_id);
}

void test_device_id_from_mac_uses_last_six_hex_chars(void)
{
    char device_id[16];

    TEST_ASSERT_EQUAL(PORT_OK,
                      mqtt_device_id_from_mac(device_id, sizeof(device_id), "aabbccddeeff"));
    TEST_ASSERT_EQUAL_STRING("ddeeff", device_id);
}

void test_topic_format_cmd_wildcard_subscription(void)
{
    char topic[64];

    TEST_ASSERT_EQUAL(PORT_OK, mqtt_topic_format(topic, sizeof(topic), "a4cf12", "cmd/#"));
    TEST_ASSERT_EQUAL_STRING("petfeeder/a4cf12/cmd/#", topic);
}

void test_topic_format_rejects_buffer_too_small(void)
{
    char topic[16];

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      mqtt_topic_format(topic, sizeof(topic), "a4cf12", "cmd/dispense"));
}

void test_device_id_from_mac_rejects_short_mac(void)
{
    char device_id[16];

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      mqtt_device_id_from_mac(device_id, sizeof(device_id), "abc"));
}
