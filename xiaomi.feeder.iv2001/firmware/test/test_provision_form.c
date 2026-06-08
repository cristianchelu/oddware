/* Tests: spec/30-processes/provisioning-flow.md (HTTP interface) */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "provision_form.h"

void test_parse_uses_ssid_pick_when_manual_empty(void)
{
    provision_input_t input;
    const char *body = "wifi_ssid_pick=HomeNet&wifi_pass=password1&mqtt_host=broker.local";

    TEST_ASSERT_EQUAL(PORT_OK, provision_form_parse_urlencoded(body, strlen(body), &input));
    TEST_ASSERT_EQUAL_STRING("HomeNet", input.wifi_ssid);
}

void test_parse_manual_ssid_overrides_pick(void)
{
    provision_input_t input;
    const char *body = "wifi_ssid_pick=OpenNet&wifi_ssid=HomeNet&mqtt_host=broker.local";

    TEST_ASSERT_EQUAL(PORT_OK, provision_form_parse_urlencoded(body, strlen(body), &input));
    TEST_ASSERT_EQUAL_STRING("HomeNet", input.wifi_ssid);
}

void test_parse_pick_survives_empty_manual_field(void)
{
    provision_input_t input;
    const char *body = "wifi_ssid_pick=HomeNet&wifi_ssid=&mqtt_host=broker.local";

    TEST_ASSERT_EQUAL(PORT_OK, provision_form_parse_urlencoded(body, strlen(body), &input));
    TEST_ASSERT_EQUAL_STRING("HomeNet", input.wifi_ssid);
}

void test_parse_manual_before_pick_does_not_get_overwritten(void)
{
    provision_input_t input;
    const char *body = "wifi_ssid=HomeNet&wifi_ssid_pick=OpenNet&mqtt_host=broker.local";

    TEST_ASSERT_EQUAL(PORT_OK, provision_form_parse_urlencoded(body, strlen(body), &input));
    TEST_ASSERT_EQUAL_STRING("HomeNet", input.wifi_ssid);
}

void test_parse_empty_pick_and_manual_leaves_ssid_empty(void)
{
    provision_input_t input;
    const char *body = "wifi_ssid_pick=&wifi_ssid=&mqtt_host=broker.local";

    TEST_ASSERT_EQUAL(PORT_OK, provision_form_parse_urlencoded(body, strlen(body), &input));
    TEST_ASSERT_EQUAL_STRING("", input.wifi_ssid);
}

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

static void assert_invalid_mqtt_port_body(const char *port_field)
{
    provision_input_t input;
    char body[128];

    snprintf(body, sizeof(body),
             "wifi_ssid=HomeNet&wifi_pass=password1&mqtt_host=broker.local&mqtt_port=%s",
             port_field);
    TEST_ASSERT_EQUAL(PORT_OK, provision_form_parse_urlencoded(body, strlen(body), &input));
    TEST_ASSERT_TRUE(input.mqtt_port_set);
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, provision_form_validate(&input));
}

void test_validate_rejects_non_numeric_mqtt_port(void)
{
    assert_invalid_mqtt_port_body("bla");
}

void test_validate_rejects_mqtt_port_zero(void)
{
    assert_invalid_mqtt_port_body("0");
}

void test_validate_rejects_mqtt_port_above_max(void)
{
    assert_invalid_mqtt_port_body("65536");
}

void test_validate_rejects_mqtt_port_with_suffix(void)
{
    assert_invalid_mqtt_port_body("1883abc");
}

void test_wifi_fail_message_includes_ssid(void)
{
    char msg[160];

    TEST_ASSERT_GREATER_THAN(0, provision_form_wifi_fail_message("HomeNet", msg, sizeof(msg)));
    TEST_ASSERT_NOT_NULL(strstr(msg, "HomeNet"));
    TEST_ASSERT_NOT_NULL(strstr(msg, "Could not connect"));
}

void test_render_empty_scan_list_still_has_select(void)
{
    char html[4096];
    provision_scan_list_t scan;
    size_t len;

    provision_scan_list_clear(&scan);
    len = provision_form_render(NULL, NULL, &scan, html, sizeof(html));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(html, "name=\"wifi_ssid_pick\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "— Select a network —"));
    TEST_ASSERT_NULL(strstr(html, " dBm)"));
}

void test_render_marks_selected_scan_option(void)
{
    char html[4096];
    provision_input_t input;
    provision_scan_list_t scan;
    size_t len;

    provision_scan_list_clear(&scan);
    scan.aps[0].rssi = -45;
    strcpy(scan.aps[0].ssid, "HomeNet");
    scan.aps[1].rssi = -60;
    strcpy(scan.aps[1].ssid, "OpenNet");
    scan.count = 2;

    provision_input_init(&input);
    strcpy(input.wifi_ssid, "OpenNet");
    strcpy(input.mqtt_host, "broker.local");

    len = provision_form_render(&input, NULL, &scan, html, sizeof(html));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"OpenNet\" selected"));
    TEST_ASSERT_NULL(strstr(html, "value=\"HomeNet\" selected"));
}

void test_render_escapes_special_chars_in_scan_ssid(void)
{
    char html[4096];
    provision_scan_list_t scan;
    size_t len;

    provision_scan_list_clear(&scan);
    scan.aps[0].rssi = -50;
    strcpy(scan.aps[0].ssid, "Tom&Jerry\"");
    scan.count = 1;

    len = provision_form_render(NULL, NULL, &scan, html, sizeof(html));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(html, "Tom&amp;Jerry&quot;"));
    TEST_ASSERT_NULL(strstr(html, "Tom&Jerry\""));
}

void test_wifi_fail_message_escapes_ssid(void)
{
    char msg[160];

    TEST_ASSERT_GREATER_THAN(0, provision_form_wifi_fail_message("Net\"A\"", msg, sizeof(msg)));
    TEST_ASSERT_NOT_NULL(strstr(msg, "&quot;"));
    TEST_ASSERT_NULL(strstr(msg, "Net\"A\""));
}

void test_render_includes_scan_select_and_refresh_link(void)
{
    char html[4096];
    provision_input_t input;
    provision_scan_list_t scan;
    size_t len;

    provision_scan_list_clear(&scan);
    scan.aps[0].rssi = -45;
    strcpy(scan.aps[0].ssid, "HomeNet");
    scan.aps[1].rssi = -60;
    strcpy(scan.aps[1].ssid, "OpenNet");
    scan.count = 2;

    provision_input_init(&input);
    strcpy(input.mqtt_host, "broker.local");

    len = provision_form_render(&input, NULL, &scan, html, sizeof(html));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(html, "name=\"wifi_ssid_pick\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "HomeNet (-45 dBm)"));
    TEST_ASSERT_NOT_NULL(strstr(html, "OpenNet (-60 dBm)"));
    TEST_ASSERT_NOT_NULL(strstr(html, "Refresh network list"));
    TEST_ASSERT_NOT_NULL(strstr(html, "type manually"));
}

void test_render_wifi_fail_repopulates_fields_and_alert(void)
{
    char html[4096];
    char msg[160];
    provision_input_t input;
    size_t len;

    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    strcpy(input.wifi_pass, "password1");
    strcpy(input.mqtt_host, "broker.local");
    input.mqtt_port_set = true;
    input.mqtt_port = 1883;
    strcpy(input.mqtt_user, "alice");
    strcpy(input.mqtt_pass, "secret");

    provision_form_wifi_fail_message(input.wifi_ssid, msg, sizeof(msg));
    len = provision_form_render(&input, msg, NULL, html, sizeof(html));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(html, "action=\"/provision.cgi\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "role=\"alert\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "HomeNet"));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"HomeNet\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"password1\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"broker.local\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"1883\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"alice\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"secret\""));
}

void test_render_mqtt_fail_repopulates_fields_and_alert(void)
{
    char html[4096];
    provision_input_t input;
    size_t len;

    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    strcpy(input.wifi_pass, "password1");
    strcpy(input.mqtt_host, "broker.local");
    input.mqtt_port_set = true;
    input.mqtt_port = 1883;

    len = provision_form_render(&input, PROVISION_MSG_MQTT_FAIL, NULL, html, sizeof(html));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(html, "role=\"alert\""));
    TEST_ASSERT_NOT_NULL(strstr(html, PROVISION_MSG_MQTT_FAIL));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"HomeNet\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"password1\""));
    TEST_ASSERT_NOT_NULL(strstr(html, "value=\"broker.local\""));
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
