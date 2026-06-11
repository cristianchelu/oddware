/* Tests: spec/30-processes/uart-console.md (MQTT connect/disconnect), mqtt-protocol.md */

#include "unity.h"

#include "config_keys.h"
#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "fake_wifi_port.h"
#include <string.h>

#include "app.h"
#include "app_event.h"
#include "display_presentation.h"
#include "fake_display_port.h"
#include "mqtt_client.h"
#include "mqtt_client_test.h"
#include "mqtt_cred.h"

static void seed_broker_config(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_host(cfg, "broker.local"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_port(cfg, 1883));
}

static void post_display_tick(uint32_t now_ms)
{
    app_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_DISPLAY_TICK;
    ev.u.display_tick.now_ms = now_ms;
    TEST_ASSERT_TRUE(app_event_post(&ev));
}

static void setup_wifi_up(void)
{
    fake_wifi_port_reset();
    fake_wifi_port_set_sta_up(true, true);
    mqtt_client_notify_wifi_ready();
}

void test_client_idle_when_not_armed(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    mqtt_client_step();
    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->connect_calls);
}

void test_request_connect_posts_connecting_session(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    app_test_reset();
    fake_display_port_reset();
    display_presentation_reset();
    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    app_step();
    post_display_tick(0u);
    app_step();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x01u, grids[4]);
}

void test_request_connect_requires_wifi(void)
{
    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();

    TEST_ASSERT_FALSE(mqtt_client_request_connect());
    TEST_ASSERT_FALSE(mqtt_client_connect_in_progress());
}

void test_connect_subscribes_and_publishes_online(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    mqtt_client_step();
    app_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->connect_calls);
    TEST_ASSERT_EQUAL_UINT(1, mqtt->subscribe_calls);
    TEST_ASSERT_EQUAL_UINT(2, mqtt->publish_calls);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/cmd/#", mqtt->last_subscribe_topic);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/state", mqtt->prior_publish_topic);
    TEST_ASSERT_EQUAL_STRING("{\"online\": true, \"bank\": \"A\"}", mqtt->prior_publish_payload);
    TEST_ASSERT_EQUAL_STRING("petfeeder/ddeeff/ota/status", mqtt->last_publish_topic);
    TEST_ASSERT_EQUAL_STRING("{\"state\": \"idle\", \"pct\": 0, \"error\": \"\"}",
                             mqtt->last_publish_payload);
    TEST_ASSERT_TRUE(mqtt->connected);
}

void test_connected_session_survives_yield_steps(void)
{
    const fake_mqtt_port_state_t *mqtt;
    int i;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();
    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    mqtt_client_step();

    for (i = 0; i < 20; i++) {
        mqtt_client_step();
    }

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->connect_calls);
    TEST_ASSERT_EQUAL_UINT(0, mqtt->disconnect_calls);
    TEST_ASSERT_TRUE(mqtt->connected);
}

void test_stop_disarms_and_disconnects(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();
    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    mqtt_client_step();

    mqtt_client_stop();
    mqtt_client_step();
    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->disconnect_calls);
    TEST_ASSERT_EQUAL_UINT(1, mqtt->connect_calls);
    TEST_ASSERT_FALSE(mqtt->connected);
}

void test_connect_failure_honors_backoff(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();
    fake_mqtt_port_set_fail_next_connect(true);

    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->connect_calls);

    mqtt_client_step();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->connect_calls);

    fake_time_advance_ms(2000);
    mqtt_client_step();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->connect_calls);
}

void test_bootstrap_without_autoconnect_flag_does_not_connect(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    mqtt_client_step();
    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->connect_calls);
}

void test_mqtt_stop_clears_connect_in_progress(void)
{
    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    TEST_ASSERT_TRUE(mqtt_client_connect_in_progress());

    mqtt_client_stop();
    TEST_ASSERT_FALSE(mqtt_client_connect_in_progress());

    mqtt_client_step();
    TEST_ASSERT_EQUAL_UINT(0, fake_mqtt_port_state()->connect_calls);
}

void test_mqtt_connect_posts_connected_session_indicator(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    app_test_reset();
    fake_display_port_reset();
    display_presentation_reset();
    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    mqtt_client_step();
    app_step();
    post_display_tick(0u);
    app_step();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[4]);
    TEST_ASSERT_FALSE(mqtt_client_connect_in_progress());
}

void test_stored_host_autoconnects_on_wifi_ready(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_start();
    setup_wifi_up();

    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->connect_calls);
}

/* Regression: OTA must not spawn MQTT reconnect while HTTP download runs. */
void test_suspend_for_ota_blocks_pending_connect(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    mqtt_client_suspend_for_ota();
    mqtt_client_step();
    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->connect_calls);
    TEST_ASSERT_FALSE(mqtt_client_connect_in_progress());
}

void test_suspend_for_ota_disconnects_without_reconnect(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    mqtt_client_step();

    mqtt_client_suspend_for_ota();
    mqtt_client_step();
    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->connect_calls);
    TEST_ASSERT_EQUAL_UINT(1, mqtt->disconnect_calls);
    TEST_ASSERT_FALSE(mqtt->connected);
}
