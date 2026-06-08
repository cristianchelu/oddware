/* Tests: spec/30-processes/provisioning-flow.md (HTTP interface) */

#include <string.h>

#include "unity.h"

#include "provision_form.h"

void test_parse_valid_form_fields(void)
{
    provision_input_t input;
    const char *body = "wifi_ssid=HomeNet&wifi_pass=password1&mqtt_host=broker.local"
                       "&mqtt_port=1883&mqtt_user=alice&mqtt_pass=secret&device_id=feeder1";

    TEST_ASSERT_EQUAL(PORT_OK, provision_form_parse_urlencoded(body, strlen(body), &input));
    TEST_ASSERT_EQUAL_STRING("HomeNet", input.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("password1", input.wifi_pass);
    TEST_ASSERT_EQUAL_STRING("broker.local", input.mqtt_host);
    TEST_ASSERT_TRUE(input.mqtt_port_set);
    TEST_ASSERT_EQUAL(1883, input.mqtt_port);
    TEST_ASSERT_EQUAL_STRING("alice", input.mqtt_user);
    TEST_ASSERT_EQUAL_STRING("secret", input.mqtt_pass);
    TEST_ASSERT_EQUAL_STRING("feeder1", input.device_id);
}

void test_parse_url_encoding_and_open_wifi(void)
{
    provision_input_t input;
    const char *body = "wifi_ssid=Open%20Net&wifi_pass=&mqtt_host=10.0.0.5";

    TEST_ASSERT_EQUAL(PORT_OK, provision_form_parse_urlencoded(body, strlen(body), &input));
    TEST_ASSERT_EQUAL_STRING("Open Net", input.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("", input.wifi_pass);
    TEST_ASSERT_EQUAL_STRING("10.0.0.5", input.mqtt_host);
    TEST_ASSERT_FALSE(input.mqtt_port_set);
}

void test_validate_rejects_bad_wifi_password(void)
{
    provision_input_t input;

    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    strcpy(input.wifi_pass, "short");
    strcpy(input.mqtt_host, "broker.local");

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, provision_form_validate(&input));
}

void test_validate_accepts_open_wifi_and_default_port(void)
{
    provision_input_t input;

    provision_input_init(&input);
    strcpy(input.wifi_ssid, "OpenNet");
    strcpy(input.mqtt_host, "broker.local");

    TEST_ASSERT_EQUAL(PORT_OK, provision_form_validate(&input));
}

void test_render_includes_form_action_and_error_message(void)
{
    char html[2048];
    provision_input_t input;
    size_t len;

    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    strcpy(input.mqtt_host, "broker.local");

    len = provision_form_render(&input, PROVISION_MSG_WIFI_FAIL, html, sizeof(html));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(html, "action=\"/provision.cgi\""));
    TEST_ASSERT_NOT_NULL(strstr(html, PROVISION_MSG_WIFI_FAIL));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"HomeNet\""));
}

void test_render_success_has_meta_refresh(void)
{
    char html[512];
    size_t len;

    len = provision_form_render_success(html, sizeof(html));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(html, "http-equiv=\"refresh\""));
    TEST_ASSERT_NOT_NULL(strstr(html, PROVISION_MSG_SUCCESS));
}
