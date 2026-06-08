/* Tests: spec/30-processes/ota-flow.md, spec/30-processes/mqtt-protocol.md */

#include <string.h>

#include "unity.h"

#include "fake_mqtt_port.h"
#include "fake_ota_port.h"
#include "ota_client.h"

#define TEST_DEVICE_ID "ddeeff"

static void setup_ota_client_connected(void)
{
    fake_mqtt_port_reset();
    fake_ota_port_reset();

    fake_mqtt_port_get()->connect(NULL);
    ota_client_set_device_id(TEST_DEVICE_ID);
    ota_client_start();
}

void test_ota_cmd_valid_starts_download(void)
{
    const fake_mqtt_port_state_t *mqtt;
    const fake_ota_port_state_t *ota;
    const char *topic = "petfeeder/ddeeff/cmd/ota";
    const char *payload = "{\"url\":\"http://10.0.0.5/fw.bin\"}";

    setup_ota_client_connected();

    ota_client_on_mqtt_message(topic, payload, strlen(payload));

    mqtt = fake_mqtt_port_state();
    ota = fake_ota_port_state();

    TEST_ASSERT_EQUAL_UINT(1, ota->start_calls);
    TEST_ASSERT_EQUAL_STRING("http://10.0.0.5/fw.bin", ota->last_url);
    TEST_ASSERT_FALSE(ota->last_has_sha512);
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/ota/status", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("{\"state\": \"downloading\", \"pct\": 0, \"error\": \"\"}",
                             mqtt->last_publish_payload);
}

void test_ota_cmd_already_in_progress(void)
{
    const fake_mqtt_port_state_t *mqtt;
    const fake_ota_port_state_t *ota;
    const char *topic = "petfeeder/ddeeff/cmd/ota";
    const char *payload = "{\"url\":\"http://10.0.0.5/fw.bin\"}";

    setup_ota_client_connected();
    fake_ota_port_set_start_result(PORT_ERR_BUSY);

    ota_client_on_mqtt_message(topic, payload, strlen(payload));

    mqtt = fake_mqtt_port_state();
    ota = fake_ota_port_state();

    TEST_ASSERT_EQUAL_UINT(1, ota->start_calls);
    TEST_ASSERT_EQUAL_STRING("{\"state\": \"error\", \"pct\": 0, \"error\": \"already_in_progress\"}",
                             mqtt->last_publish_payload);
}

void test_ota_cmd_invalid_url(void)
{
    const fake_mqtt_port_state_t *mqtt;
    const fake_ota_port_state_t *ota;
    const char *topic = "petfeeder/ddeeff/cmd/ota";
    const char *payload = "{\"url\":\"file:///tmp/fw.bin\"}";

    setup_ota_client_connected();

    ota_client_on_mqtt_message(topic, payload, strlen(payload));

    mqtt = fake_mqtt_port_state();
    ota = fake_ota_port_state();

    TEST_ASSERT_EQUAL_UINT(0, ota->start_calls);
    TEST_ASSERT_EQUAL_STRING("{\"state\": \"error\", \"pct\": 0, \"error\": \"invalid_url\"}",
                             mqtt->last_publish_payload);
}

void test_ota_cmd_wrong_topic_ignored(void)
{
    const fake_mqtt_port_state_t *mqtt;
    const fake_ota_port_state_t *ota;
    const char *topic = "petfeeder/ddeeff/cmd/dispense";
    const char *payload = "{\"url\":\"http://10.0.0.5/fw.bin\"}";

    setup_ota_client_connected();

    ota_client_on_mqtt_message(topic, payload, strlen(payload));

    mqtt = fake_mqtt_port_state();
    ota = fake_ota_port_state();

    TEST_ASSERT_EQUAL_UINT(0, ota->start_calls);
    TEST_ASSERT_EQUAL_UINT(0, mqtt->publish_calls);
}

void test_ota_on_mqtt_connected_publishes_idle(void)
{
    const fake_mqtt_port_state_t *mqtt;

    setup_ota_client_connected();

    ota_client_on_mqtt_connected();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/ota/status", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("{\"state\": \"idle\", \"pct\": 0, \"error\": \"\"}",
                             mqtt->last_publish_payload);
}

void test_ota_progress_callback_publishes_status(void)
{
    const fake_mqtt_port_state_t *mqtt;
    ota_progress_t progress = {
        .status = OTA_STATUS_DOWNLOADING,
        .pct = 50,
        .error = NULL,
    };

    setup_ota_client_connected();

    fake_ota_port_emit_progress(&progress);

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("{\"state\": \"downloading\", \"pct\": 50, \"error\": \"\"}",
                             mqtt->last_publish_payload);
}
