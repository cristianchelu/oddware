/* Tests: spec/30-processes/web-ui.md */

#include <string.h>

#include "unity.h"

#include "bowl_mass_present.h"
#include "fake_config_port.h"
#include "fake_time_port.h"
#include "feed_config.h"
#include "mqtt_battery.h"
#include "mqtt_bowl_weight.h"
#include "mqtt_hopper.h"
#include "mqtt_mains.h"
#include "mqtt_state.h"
#include "schedule.h"
#include "schedule_test_epochs.h"
#include "time_sync.h"
#include "tz_rule.h"
#include "web_api.h"

static void setup_status_synced(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    fake_time_port_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    schedule_test_reset();

    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg, "UTC0"));
    tz_rule_init();

    fake_time_port_set_epoch(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    time_sync_poll(1000u);
    TEST_ASSERT_TRUE(time_sync_is_valid());

    schedule_init();
}

static void setup_status_telemetry(void)
{
    mqtt_bowl_weight_test_reset();
    mqtt_state_test_reset();
    mqtt_hopper_test_reset();
    mqtt_battery_test_reset();
    mqtt_mains_test_reset();

    mqtt_bowl_weight_sync(BOWL_MASS_KNOWN, 423, true);
    mqtt_state_sync(false);
    mqtt_hopper_sync(HOPPER_LEVEL_STATE_LOW);
    mqtt_battery_sync(true, 85u, true);
    mqtt_mains_sync(true);
}

void test_web_api_classify_status_get(void)
{
    TEST_ASSERT_EQUAL(WEB_API_GET_STATUS, web_api_classify("GET", "/api/status"));
}

void test_web_api_classify_schedule_state_get(void)
{
    TEST_ASSERT_EQUAL(WEB_API_GET_SCHEDULE_STATE,
                      web_api_classify("GET", "/api/schedule/state"));
}

void test_web_api_classify_schedule_set_post(void)
{
    TEST_ASSERT_EQUAL(WEB_API_POST_SCHEDULE_SET,
                      web_api_classify("POST", "/api/schedule/set"));
}

void test_web_api_classify_unknown_path(void)
{
    TEST_ASSERT_EQUAL(WEB_API_ROUTE_UNKNOWN, web_api_classify("GET", "/api/nope"));
}

void test_web_api_handle_get_schedule_state(void)
{
    char buf[512];
    int written;

    fake_config_port_reset();
    schedule_test_reset();
    schedule_init();

    written = web_api_handle_get(WEB_API_GET_SCHEDULE_STATE, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"schedule\""));
}

void test_web_api_handle_post_schedule_set(void)
{
    char resp[128];
    const char *payload =
        "{\"hour\":7,\"min\":15,\"repeat_days\":[0,1,2,3,4,5,6],\"g\":40,\"enabled\":true}";
    int written;

    fake_config_port_reset();
    schedule_test_reset();
    schedule_init();

    written = web_api_handle_post(WEB_API_POST_SCHEDULE_SET,
                                  payload,
                                  strlen(payload),
                                  resp,
                                  sizeof(resp));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(resp, "\"ok\":true"));
    TEST_ASSERT_EQUAL(1, schedule_slot_count());
}

void test_web_api_handle_get_feed_mode(void)
{
    char buf[32];
    int written;

    fake_config_port_reset();
    feed_config_mode_set(DISPENSE_MODE_COMPENSATED);

    written = web_api_handle_get(WEB_API_GET_FEED_MODE, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_EQUAL_STRING("compensated", buf);
}

void test_web_api_handle_get_feed_overfill(void)
{
    char buf[64];
    int written;

    fake_config_port_reset();
    TEST_ASSERT_TRUE(feed_config_overfill_enabled_set(true));
    TEST_ASSERT_TRUE(feed_config_overfill_threshold_g_set(40u));

    written = web_api_handle_get(WEB_API_GET_FEED_OVERFILL, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_EQUAL_STRING("{\"enabled\":true,\"threshold_g\":40}", buf);
}

void test_web_api_handle_post_feed_overfill(void)
{
    char resp[64];
    int written;

    fake_config_port_reset();
    written = web_api_handle_post(WEB_API_POST_FEED_OVERFILL,
                                  "{\"enabled\":true,\"threshold_g\":35}",
                                  strlen("{\"enabled\":true,\"threshold_g\":35}"),
                                  resp,
                                  sizeof(resp));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(resp, "\"ok\":true"));
    TEST_ASSERT_TRUE(feed_config_overfill_enabled_get());
    TEST_ASSERT_EQUAL_UINT8(35u, feed_config_overfill_threshold_g_get());
}

void test_web_api_handle_get_status_telemetry(void)
{
    char buf[1024];
    int written;

    setup_status_synced();
    setup_status_telemetry();

    written = web_api_handle_get(WEB_API_GET_STATUS, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"time_synced\":true"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"hopper\":\"low\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"bowl_weight\":\"42.3\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"bowl_error\":false"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"battery\":\"85\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"mains\":\"ON\""));
}
