/* Tests: spec/30-processes/auto-tare.md, monitoring.md */

#include <string.h>

#include "unity.h"

#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "auto_tare.h"
#include "fake_display_port.h"
#include "fake_mqtt_port.h"
#include "fake_weight_port.h"
#include "mqtt_bowl_weight.h"
#include "mqtt_client_test.h"
#include "mqtt_outbox.h"
#include "tm1637.h"
#include "weight_units.h"

#define TEST_DEVICE_ID "ddeeff"

extern void fake_app_event_q_reset(void);

static void post_event(app_event_type_t type)
{
    app_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    app_step();
}

static void post_timer_tick(void)
{
    post_event(EVT_TIMER_TICK);
}

static void post_display_tick(uint32_t now_ms)
{
    app_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_DISPLAY_TICK;
    ev.u.display_tick.now_ms = now_ms;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    app_step();
}

static void app_auto_tare_test_boot_idle(void)
{
    post_event(EVT_APP_BOOT);
    post_timer_tick();
    post_timer_tick();
}

static void app_auto_tare_test_anchor_quiet(weight_dg_t dg, uint32_t base_ms)
{
    fake_weight_port_set_read_dg(dg);
    for (uint32_t i = 0u; i < 4u; i++) {
        post_display_tick(base_ms + i * 500u);
    }
    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_EQUAL_INT32(dg, auto_tare_stable_dg());
}

static void app_auto_tare_test_setup_mqtt(void)
{
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    fake_mqtt_port_get()->connect(NULL);
    mqtt_client_test_reset();
    mqtt_client_test_set_device_id(TEST_DEVICE_ID);
    mqtt_bowl_weight_test_reset();
    mqtt_bowl_weight_set_device_id(TEST_DEVICE_ID);
}

static void app_auto_tare_test_drain_mqtt(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        (void)mqtt_outbox_drain_one(mqtt);
    }
}

static void app_auto_tare_test_reset(void)
{
    fake_weight_port_reset();
    fake_display_port_reset();
    fake_app_event_q_reset();
    app_test_reset();
    app_event_port_init();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
}

void test_app_presented_weight_holds_during_drift(void)
{
    uint8_t grids_before[TM1637_GRID_COUNT];
    uint8_t grids_after[TM1637_GRID_COUNT];

    app_auto_tare_test_reset();
    fake_weight_port_set_read_dg(120);
    app_auto_tare_test_boot_idle();
    app_auto_tare_test_anchor_quiet(120, 500u);

    post_display_tick(2500u);
    fake_display_port_last_grids(grids_before);

    fake_weight_port_set_read_dg(119);
    post_display_tick(3000u);
    fake_display_port_last_grids(grids_after);

    TEST_ASSERT_EQUAL_INT32(120, auto_tare_present_dg(119, true));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(grids_before, grids_after, TM1637_GRID_COUNT);
}

void test_app_bowl_missing_invalidates_auto_tare(void)
{
    app_auto_tare_test_reset();
    fake_weight_port_set_read_dg(200);
    app_auto_tare_test_boot_idle();
    app_auto_tare_test_anchor_quiet(200, 500u);

    fake_weight_port_set_read_dg(-1000);
    post_display_tick(3000u);

    TEST_ASSERT_TRUE(auto_tare_pending_calibration());
    TEST_ASSERT_FALSE(auto_tare_stable_valid());
}

void test_app_mqtt_bowl_weight_holds_presented_during_drift(void)
{
    const fake_mqtt_port_state_t *mqtt;
    unsigned publishes_before;
    unsigned publishes_after;

    app_auto_tare_test_reset();
    app_auto_tare_test_setup_mqtt();
    fake_weight_port_set_read_dg(120);
    app_auto_tare_test_boot_idle();
    app_auto_tare_test_anchor_quiet(120, 500u);

    post_display_tick(2500u);
    app_auto_tare_test_drain_mqtt();
    mqtt = fake_mqtt_port_state();
    publishes_before = mqtt->publish_calls;
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "12.0"));

    fake_weight_port_set_read_dg(119);
    post_display_tick(3000u);
    app_auto_tare_test_drain_mqtt();
    mqtt = fake_mqtt_port_state();
    publishes_after = mqtt->publish_calls;

    TEST_ASSERT_EQUAL_INT32(120, auto_tare_present_dg(119, true));
    TEST_ASSERT_EQUAL_UINT(publishes_before, publishes_after);
}
