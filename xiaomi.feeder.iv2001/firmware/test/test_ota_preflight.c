/* Tests: spec/30-processes/ota-flow.md § Pre-download memory reclaim */

#include "unity.h"

#include <string.h>

#include "config_keys.h"
#include "console_mux.h"
#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "fake_wifi_port.h"
#include "mqtt_client.h"
#include "mqtt_client_test.h"
#include "mqtt_cred.h"
#include "ota_preflight.h"
#include "remote_cli.h"
#include "app_cli_ota.h"

static void seed_broker_config(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_host(cfg, "broker.local"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_port(cfg, 1883));
}

static void setup_wifi_up(void)
{
    fake_wifi_port_reset();
    fake_wifi_port_set_sta_up(true, true);
    mqtt_client_notify_wifi_ready();
}

static void remote_cli_test_reset_all(void)
{
    fake_time_reset();
    console_mux_test_reset();
    remote_cli_test_reset();
}

void test_preflight_suspend_mqtt_blocks_connect(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    TEST_ASSERT_EQUAL(PORT_OK, ota_preflight_suspend_idle_tasks());
    TEST_ASSERT_TRUE(mqtt_client_test_is_suspended());

    TEST_ASSERT_FALSE(mqtt_client_request_connect());
    mqtt_client_step();
    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(0, mqtt->connect_calls);
    TEST_ASSERT_FALSE(mqtt_client_connect_in_progress());
}

void test_preflight_suspend_remote_cli_ends_session(void)
{
    remote_cli_test_reset_all();
    app_cli_test_reset();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    TEST_ASSERT_TRUE(console_mux_remote_active());

    TEST_ASSERT_EQUAL(PORT_OK, ota_preflight_suspend_idle_tasks());

    TEST_ASSERT_FALSE(remote_cli_test_session_active());
    TEST_ASSERT_FALSE(console_mux_remote_active());
    TEST_ASSERT_TRUE(remote_cli_test_is_suspended_for_ota());
    TEST_ASSERT_TRUE(app_cli_test_is_suspended_for_ota());
}

void test_preflight_resume_restores_app_cli(void)
{
    app_cli_test_reset();

    TEST_ASSERT_EQUAL(PORT_OK, ota_preflight_suspend_idle_tasks());
    TEST_ASSERT_TRUE(app_cli_test_is_suspended_for_ota());

    ota_preflight_resume_idle_tasks();
    TEST_ASSERT_FALSE(app_cli_test_is_suspended_for_ota());
}

void test_preflight_resume_restores_remote_cli(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_EQUAL(PORT_OK, ota_preflight_suspend_idle_tasks());
    ota_preflight_resume_idle_tasks();

    TEST_ASSERT_FALSE(remote_cli_test_is_suspended_for_ota());
    TEST_ASSERT_TRUE(console_mux_try_remote());
    console_mux_release_remote();
}

void test_preflight_resume_restores_mqtt_connect(void)
{
    const fake_mqtt_port_state_t *mqtt;

    fake_time_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();
    setup_wifi_up();

    TEST_ASSERT_TRUE(mqtt_client_request_connect());
    mqtt_client_step();

    TEST_ASSERT_EQUAL(PORT_OK, ota_preflight_suspend_idle_tasks());
    ota_preflight_resume_idle_tasks();

    TEST_ASSERT_FALSE(mqtt_client_test_is_suspended());
    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(2, mqtt->connect_calls);
}
