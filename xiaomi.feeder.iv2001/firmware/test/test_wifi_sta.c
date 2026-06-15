/* Tests: spec/30-processes/uart-console.md, wifi-lifecycle.md */

#include <string.h>

#include "unity.h"

#include "app_event.h"
#include "app_event_port.h"
#include "config_keys.h"
#include "fake_config_port.h"
#include "fake_wifi_port.h"
#include "wifi_cred.h"
#include "wifi_sta.h"
#include "wifi_sta_test.h"

static void seed_wifi_cred(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_SSID, "HomeNet"));
    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_PASS, "secretpass"));
}

static bool drain_event(app_event_type_t expected)
{
    app_event_t ev;

    if (!app_event_try_receive(&ev)) {
        return false;
    }

    return ev.type == expected;
}

void test_request_connect_busy_guard(void)
{
    fake_wifi_port_reset();
    seed_wifi_cred();
    wifi_sta_test_bootstrap();

    TEST_ASSERT_TRUE(wifi_sta_request_connect());
    TEST_ASSERT_EQUAL(WIFI_STA_BUSY_CONNECT, wifi_sta_busy());
    TEST_ASSERT_FALSE(wifi_sta_request_connect());
}

void test_request_disconnect_posts_down_sequence(void)
{
    fake_wifi_port_reset();
    fake_wifi_port_set_sta_up(true, true);
    wifi_sta_test_bootstrap();

    TEST_ASSERT_TRUE(wifi_sta_request_disconnect());
    wifi_sta_test_pump();

    TEST_ASSERT_EQUAL(WIFI_STA_IDLE, wifi_sta_busy());
    TEST_ASSERT_EQUAL_UINT32(1u, fake_wifi_port_state()->disconnect_calls);
}

void test_connect_posts_ready_event(void)
{
    fake_wifi_port_reset();
    seed_wifi_cred();
    app_event_port_init();
    wifi_sta_test_bootstrap();

    TEST_ASSERT_TRUE(wifi_sta_request_connect());
    TEST_ASSERT_TRUE(drain_event(EVT_WIFI_STA_CONNECTING));

    wifi_sta_test_pump();

    {
        app_event_t ev;

        TEST_ASSERT_TRUE(app_event_try_receive(&ev));
        TEST_ASSERT_EQUAL(EVT_WIFI_STA_READY, ev.type);
        TEST_ASSERT_EQUAL_STRING("192.168.1.10", ev.u.wifi_ready.ip);
    }
}

void test_connect_failure_posts_failed_event(void)
{
    app_event_t ev;
    bool saw_failed = false;

    fake_wifi_port_reset();
    seed_wifi_cred();
    fake_wifi_port_set_wait_ready_result(PORT_ERR_IO);
    app_event_port_init();
    wifi_sta_test_bootstrap();

    TEST_ASSERT_TRUE(wifi_sta_request_connect());
    wifi_sta_test_pump();

    while (app_event_try_receive(&ev)) {
        if (ev.type == EVT_WIFI_STA_FAILED) {
            saw_failed = true;
        }
    }

    TEST_ASSERT_TRUE(saw_failed);
}
