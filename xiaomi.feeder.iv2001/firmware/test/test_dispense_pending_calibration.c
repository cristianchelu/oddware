/* Tests: spec/30-processes/dispense-cycle.md § Pre-dispense baseline, auto-tare.md */

#include <string.h>

#include "unity.h"

#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "auto_tare.h"
#include "dispense.h"
#include "fake_time.h"
#include "fake_weight_port.h"
#include "fake_motor_port.h"
#include "motor_port_provider_host.h"
#include "fake_mqtt_port.h"
#include "feeder_runtime.h"
#include "mqtt_client.h"
#include "mqtt_dispense_event.h"
#include "mqtt_outbox.h"
#include "mqtt_client_test.h"

#define TEST_DEVICE_ID "ddeeff"

extern void fake_app_event_q_reset(void);

static void pending_cal_setup_mqtt(void)
{
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    mqtt_client_test_reset();
    mqtt_client_test_set_device_id(TEST_DEVICE_ID);
    mqtt_dispense_event_set_device_id(TEST_DEVICE_ID);
}

static void pending_cal_drain_mqtt(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        (void)mqtt_outbox_drain_one(mqtt);
    }
}

static void pending_cal_test_reset(void)
{
    fake_time_reset();
    motor_port_host_reset();
    fake_motor_port_reset();
    fake_weight_port_reset();
    fake_app_event_q_reset();
    dispense_test_reset();
    app_test_reset();
    app_event_port_init();
    auto_tare_test_reset();
    feeder_runtime_test_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
}

static size_t pending_cal_read_grams_op_count(void)
{
    size_t count = 0u;
    size_t total = 0u;
    const fake_weight_op_t *ops = fake_weight_port_ops(&total);
    size_t i;

    for (i = 0u; i < total; i++) {
        if (ops[i].kind == FAKE_WEIGHT_OP_READ_GRAMS) {
            count++;
        }
    }

    return count;
}

void test_dispense_pending_calibration_skips_fresh_cache(void)
{
    pending_cal_test_reset();
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());

    app_bowl_grams_notify_read(50, true, 1u);
    fake_weight_port_set_read_grams(0);

    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(1u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());

    TEST_ASSERT_EQUAL(1u, pending_cal_read_grams_op_count());
    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_TRUE(auto_tare_stable_valid());
    TEST_ASSERT_EQUAL_INT32(0, auto_tare_stable_grams());
}

void test_dispense_wash_reinstall_quick_dispense_measures_delta(void)
{
    app_event_t ev;
    const fake_mqtt_port_state_t *mqtt;

    pending_cal_test_reset();
    pending_cal_setup_mqtt();
    TEST_ASSERT_TRUE(auto_tare_pending_calibration());

    fake_weight_port_set_read_grams(0);
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(3u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_FALSE(auto_tare_pending_calibration());

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());

    fake_weight_port_set_read_grams(30);
    dispense_poll(1000u);
    dispense_poll(1000u + DISPENSE_SETTLE_MS);

    TEST_ASSERT_FALSE(dispense_is_active());
    TEST_ASSERT_EQUAL_INT32(30, auto_tare_stable_grams());

    pending_cal_drain_mqtt();
    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"event_type\":\"success\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"grams\":30"));
}

void test_dispense_calibrated_uses_fresh_cache_without_blocking_read(void)
{
    app_bowl_grams_snapshot_t snap;

    pending_cal_test_reset();
    app_test_finish_weight_boot();
    auto_tare_anchor(100);
    app_bowl_grams_notify_read(100, true, 1u);
    TEST_ASSERT_FALSE(auto_tare_pending_calibration());
    TEST_ASSERT_TRUE(app_bowl_grams_snapshot(1u, &snap));
    TEST_ASSERT_TRUE(snap.valid);
    TEST_ASSERT_EQUAL_UINT32(0u, snap.sample_age_ms);
    TEST_ASSERT_EQUAL_INT32(100, snap.grams);

    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(1u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());

    TEST_ASSERT_EQUAL(0u, pending_cal_read_grams_op_count());
}
