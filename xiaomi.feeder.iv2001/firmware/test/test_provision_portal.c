/* Tests: spec/30-processes/provisioning-flow.md (portal GET/POST) */

#include <string.h>

#include "unity.h"

#include "provision_pending.h"
#include "provision_portal.h"
#include "provision_scan_list.h"

static provision_flow_result_t s_submit_result = PROVISION_FLOW_OK;
static provision_portal_restore_t s_restore_kind = PROVISION_PORTAL_RESTORE_NONE;
static int s_success_calls;
static int s_rescan_calls;
static provision_scan_list_t s_scan;

static provision_flow_result_t portal_test_submit(const provision_input_t *input)
{
    (void)input;
    return s_submit_result;
}

static void portal_test_request_restore(provision_portal_restore_t kind)
{
    s_restore_kind = kind;
}

static void portal_test_success(void)
{
    s_success_calls++;
}

static void portal_test_rescan(void)
{
    s_rescan_calls++;
}

static provision_portal_deps_t portal_test_deps(void)
{
    provision_portal_deps_t deps;

    memset(&deps, 0, sizeof(deps));
    deps.scan = &s_scan;
    deps.active = true;
    deps.refresh_scan = portal_test_rescan;
    deps.flow_submit = portal_test_submit;
    deps.request_restore = portal_test_request_restore;
    deps.on_success = portal_test_success;
    return deps;
}

static void portal_test_reset(void)
{
    provision_pending_clear();
    provision_scan_list_clear(&s_scan);
    s_submit_result = PROVISION_FLOW_OK;
    s_restore_kind = PROVISION_PORTAL_RESTORE_NONE;
    s_success_calls = 0;
    s_rescan_calls = 0;
}

void test_portal_get_shows_pending_after_mqtt_fail_post(void)
{
    char post_html[4096];
    char get_html[4096];
    provision_portal_deps_t deps = portal_test_deps();
    const char *body = "wifi_ssid=HomeNet&wifi_pass=password1&mqtt_host=broker.local"
                       "&mqtt_port=1883";

    portal_test_reset();
    s_submit_result = PROVISION_FLOW_MQTT_FAIL;

    TEST_ASSERT_GREATER_THAN(0,
                             provision_portal_handle_post(body,
                                                          strlen(body),
                                                          post_html,
                                                          sizeof(post_html),
                                                          &deps));
    TEST_ASSERT_EQUAL(PROVISION_PORTAL_RESTORE_AP_PORTAL, s_restore_kind);
    TEST_ASSERT_NOT_NULL(strstr(post_html, PROVISION_MSG_MQTT_FAIL));

    TEST_ASSERT_GREATER_THAN(0,
                             provision_portal_handle_get(NULL,
                                                         0,
                                                         get_html,
                                                         sizeof(get_html),
                                                         &deps));
    TEST_ASSERT_NOT_NULL(strstr(get_html, "role=\"alert\""));
    TEST_ASSERT_NOT_NULL(strstr(get_html, PROVISION_MSG_MQTT_FAIL));
    TEST_ASSERT_NOT_NULL(strstr(get_html, "value=\"HomeNet\""));
    TEST_ASSERT_NOT_NULL(strstr(get_html, "value=\"password1\""));
}

void test_portal_get_keeps_pending_until_post(void)
{
    char html[4096];
    provision_portal_deps_t deps = portal_test_deps();
    const char *body = "wifi_ssid=HomeNet&wifi_pass=password1&mqtt_host=broker.local";

    portal_test_reset();
    s_submit_result = PROVISION_FLOW_MQTT_FAIL;
    provision_portal_handle_post(body, strlen(body), html, sizeof(html), &deps);

    TEST_ASSERT_GREATER_THAN(0, provision_portal_handle_get(NULL, 0, html, sizeof(html), &deps));
    TEST_ASSERT_NOT_NULL(strstr(html, PROVISION_MSG_MQTT_FAIL));

    TEST_ASSERT_GREATER_THAN(0, provision_portal_handle_get(NULL, 0, html, sizeof(html), &deps));
    TEST_ASSERT_NOT_NULL(strstr(html, PROVISION_MSG_MQTT_FAIL));

    s_submit_result = PROVISION_FLOW_OK;
    provision_portal_handle_post(body, strlen(body), html, sizeof(html), &deps);

    TEST_ASSERT_GREATER_THAN(0, provision_portal_handle_get(NULL, 0, html, sizeof(html), &deps));
    TEST_ASSERT_NULL(strstr(html, PROVISION_MSG_MQTT_FAIL));
}

void test_portal_probe_gets_do_not_consume_pending(void)
{
    char post_html[4096];
    char probe_html[4096];
    char user_html[4096];
    provision_portal_deps_t deps = portal_test_deps();
    const char *body = "wifi_ssid=HomeNet&wifi_pass=password1&mqtt_host=broker.local"
                       "&mqtt_port=1883";

    portal_test_reset();
    s_submit_result = PROVISION_FLOW_WIFI_FAIL;

    TEST_ASSERT_GREATER_THAN(0,
                             provision_portal_handle_post(body,
                                                          strlen(body),
                                                          post_html,
                                                          sizeof(post_html),
                                                          &deps));

    TEST_ASSERT_GREATER_THAN(0,
                             provision_portal_handle_get(NULL,
                                                         0,
                                                         probe_html,
                                                         sizeof(probe_html),
                                                         &deps));
    TEST_ASSERT_NOT_NULL(strstr(probe_html, "role=\"alert\""));

    TEST_ASSERT_GREATER_THAN(0,
                             provision_portal_handle_get(NULL,
                                                         0,
                                                         user_html,
                                                         sizeof(user_html),
                                                         &deps));
    TEST_ASSERT_NOT_NULL(strstr(user_html, "role=\"alert\""));
    TEST_ASSERT_NOT_NULL(strstr(user_html, "value=\"HomeNet\""));
}

void test_portal_post_success_clears_pending_and_calls_on_success(void)
{
    char html[512];
    provision_portal_deps_t deps = portal_test_deps();
    const char *body = "wifi_ssid=HomeNet&wifi_pass=password1&mqtt_host=broker.local";

    portal_test_reset();
    provision_input_t input;
    char msg[PROVISION_PENDING_MSG_MAX];

    provision_input_init(&input);
    strcpy(input.wifi_ssid, "HomeNet");
    provision_pending_set(&input, PROVISION_MSG_MQTT_FAIL);

    TEST_ASSERT_GREATER_THAN(0,
                             provision_portal_handle_post(body,
                                                          strlen(body),
                                                          html,
                                                          sizeof(html),
                                                          &deps));
    TEST_ASSERT_EQUAL(1, s_success_calls);
    TEST_ASSERT_EQUAL(PROVISION_PORTAL_RESTORE_NONE, s_restore_kind);
    TEST_ASSERT_FALSE(provision_pending_take(&input, msg, sizeof(msg)));
}

void test_portal_post_save_fail_requests_ap_restore(void)
{
    char html[4096];
    provision_portal_deps_t deps = portal_test_deps();
    const char *body = "wifi_ssid=HomeNet&wifi_pass=password1&mqtt_host=broker.local";

    portal_test_reset();
    s_submit_result = PROVISION_FLOW_SAVE_FAIL;

    TEST_ASSERT_GREATER_THAN(0,
                             provision_portal_handle_post(body,
                                                          strlen(body),
                                                          html,
                                                          sizeof(html),
                                                          &deps));
    TEST_ASSERT_EQUAL(PROVISION_PORTAL_RESTORE_AP_PORTAL, s_restore_kind);
    TEST_ASSERT_NOT_NULL(strstr(html, PROVISION_MSG_SAVE_FAIL));
}

void test_portal_post_wifi_fail_requests_http_restore(void)
{
    char html[4096];
    provision_portal_deps_t deps = portal_test_deps();
    const char *body = "wifi_ssid=HomeNet&wifi_pass=password1&mqtt_host=broker.local";

    portal_test_reset();
    s_submit_result = PROVISION_FLOW_WIFI_FAIL;

    TEST_ASSERT_GREATER_THAN(0,
                             provision_portal_handle_post(body,
                                                          strlen(body),
                                                          html,
                                                          sizeof(html),
                                                          &deps));
    TEST_ASSERT_EQUAL(PROVISION_PORTAL_RESTORE_HTTP_ONLY, s_restore_kind);
}

void test_portal_get_rescan_when_active(void)
{
    char html[4096];
    provision_portal_deps_t deps = portal_test_deps();

    portal_test_reset();
    TEST_ASSERT_GREATER_THAN(0,
                             provision_portal_handle_get("rescan=1",
                                                         9,
                                                         html,
                                                         sizeof(html),
                                                         &deps));
    TEST_ASSERT_EQUAL(1, s_rescan_calls);
}
