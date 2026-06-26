/* Tests: spec/30-processes/dispense-cycle.md, uart-console.md § dispense */

#include <string.h>

#include "unity.h"

#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "cli_test_assert.h"
#include "feed_config.h"
#include "auto_tare.h"
#include "feeder_runtime.h"
#include "dispense.h"
#include "dispense_cli.h"
#include "display_presentation.h"
#include "fake_config_port.h"
#include "fake_display_port.h"
#include "fake_motor_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "fake_weight_port.h"
#include "weight_units.h"
#include "motor_jam.h"
#include "mqtt_client_test.h"
#include "mqtt_outbox.h"
#include "port_err.h"
#include "motor_port_provider_host.h"
#include "tm1637.h"

extern void fake_app_event_q_reset(void);

static void dispense_test_reset_all(void)
{
    fake_time_reset();
    motor_port_host_reset();
    fake_motor_port_reset();
    fake_weight_port_reset();
    fake_app_event_q_reset();
    fake_display_port_reset();
    display_presentation_reset();
    dispense_test_reset();
    dispense_cli_test_reset();
    app_test_reset();
    app_event_port_init();
    auto_tare_test_reset();
    feeder_runtime_test_reset();
}

static void dispense_test_advance_settle(uint32_t start_ms)
{
    dispense_poll(start_ms);
    dispense_poll(start_ms + DISPENSE_SETTLE_MS);
}

static void dispense_test_seed_baseline(int32_t grams, uint32_t sample_ms)
{
    weight_dg_t dg = WEIGHT_G_TO_DG(grams);

    auto_tare_anchor(dg);
    app_bowl_dg_notify_read(dg, true, sample_ms);
}

static void dispense_test_setup_mqtt(void)
{
    fake_mqtt_port_reset();
    mqtt_outbox_reset();
    mqtt_outbox_set_accepting(true);
    fake_mqtt_port_get()->connect(NULL);
    mqtt_client_test_set_device_id("ddeeff");
}

static void dispense_test_drain_mqtt(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void dispense_test_run_burst_and_settle(int32_t weight_grams, uint32_t *time_ms)
{
    app_event_t ev;

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(weight_grams));
    dispense_poll(*time_ms);
    dispense_poll(*time_ms + DISPENSE_SETTLE_MS);
    *time_ms += 20000u;
}

void test_dispense_submit_portions_posts_request_event(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK,
                      dispense_submit_portions(3u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(dispense_is_active());

    TEST_ASSERT_EQUAL(0u, fake_motor_port_burst_calls());

    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
    TEST_ASSERT_EQUAL(3u, fake_motor_port_last_pulse_target());
    TEST_ASSERT_EQUAL(MOTOR_BURST_TIMEOUT_MS, fake_motor_port_last_timeout_ms());
}

void test_dispense_submit_one_portion_pulse_target(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK,
                      dispense_submit_portions(1u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_last_pulse_target());
}

void test_dispense_submit_rejects_invalid_portions(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_INVALID, dispense_submit_portions(0u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_INVALID, dispense_submit_portions(16u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_FALSE(dispense_is_active());
    TEST_ASSERT_EQUAL(0u, fake_motor_port_burst_calls());
}

void test_dispense_submit_grams_posts_request_event(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK,
                      dispense_submit_grams(30u, DISPENSE_SOURCE_SCHEDULE));
    TEST_ASSERT_TRUE(dispense_is_active());
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(3u, fake_motor_port_last_pulse_target());
}

void test_dispense_submit_grams_rejects_invalid_range(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_INVALID,
                      dispense_submit_grams(4u, DISPENSE_SOURCE_SCHEDULE));
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_INVALID,
                      dispense_submit_grams(151u, DISPENSE_SOURCE_SCHEDULE));
}

void test_dispense_submit_busy_while_job_active(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(dispense_is_active());
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_BUSY, dispense_submit_portions(1u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
}

void test_dispense_submit_busy_when_motor_active(void)
{
    dispense_test_reset_all();
    fake_motor_port_set_active(true);
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_BUSY, dispense_submit_portions(1u, DISPENSE_SOURCE_MQTT));
}

void test_dispense_job_completes_on_burst_done(void)
{
    app_event_t ev;

    dispense_test_reset_all();
    dispense_test_seed_baseline(100, 0u);
    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(128));
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(3u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());

    dispense_test_advance_settle(1000u);
    TEST_ASSERT_FALSE(dispense_is_active());
}

void test_dispense_job_aborts_on_motor_fault(void)
{
    app_event_t ev;

    dispense_test_reset_all();
    dispense_test_seed_baseline(100, 0u);
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());

    ev.type = EVT_MOTOR_FAULT;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());

    dispense_test_advance_settle(2000u);
    TEST_ASSERT_FALSE(dispense_is_active());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
}

void test_dispense_stays_active_when_motor_idle_without_completion_event(void)
{
    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());

    fake_motor_port_set_active(false);
    dispense_poll(5000u);

    TEST_ASSERT_TRUE(dispense_is_active());
}

void test_dispense_stays_active_before_async_motor_start(void)
{
    dispense_test_reset_all();
    fake_motor_port_set_defer_burst_active(true);
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(1u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());

    dispense_poll(1000u);
    TEST_ASSERT_TRUE(dispense_is_active());
}

void test_dispense_measured_delta_clamps_negative(void)
{
    app_event_t ev;

    dispense_test_reset_all();
    dispense_test_seed_baseline(200, 0u);
    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(150));
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(1u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    dispense_test_advance_settle(1000u);

    TEST_ASSERT_EQUAL_UINT8(1u, dispense_test_zero_delta_streak());
}

void test_dispense_job_blinks_dispensing_indicator(void)
{
    app_event_t ev;
    uint32_t now_ms = 0u;
    bool saw_on = false;
    bool saw_off = false;
    uint8_t grids[TM1637_GRID_COUNT];

    dispense_test_reset_all();
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(2u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());

    for (uint32_t step = 0u; step <= 24u; step++) {
        memset(&ev, 0, sizeof(ev));
        ev.type = EVT_DISPLAY_TICK;
        ev.u.display_tick.now_ms = now_ms;
        TEST_ASSERT_TRUE(app_event_post(&ev));
        TEST_ASSERT_TRUE(app_step());

        fake_display_port_last_grids(grids);
        if ((grids[3] & 0x04u) != 0u) {
            saw_on = true;
        } else {
            saw_off = true;
        }
        now_ms += 50u;
    }

    TEST_ASSERT_TRUE_MESSAGE(saw_on, "dispensing icon should turn on during job");
    TEST_ASSERT_TRUE_MESSAGE(saw_off, "dispensing icon should turn off during job");
}

void test_app_prioritizes_burst_done_before_display_tick(void)
{
    app_event_t ev;

    dispense_test_reset_all();
    dispense_test_seed_baseline(100, 0u);
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK, dispense_submit_portions(1u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());
    fake_motor_port_set_active(true);

    ev.type = EVT_DISPLAY_TICK;
    ev.u.display_tick.now_ms = 1000u;
    TEST_ASSERT_TRUE(app_event_post(&ev));

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));

    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());

    dispense_test_advance_settle(1000u);
    TEST_ASSERT_FALSE(dispense_is_active());
}

void test_dispense_cli_parse_portions_valid_range(void)
{
    uint8_t portions;

    TEST_ASSERT_EQUAL(PORT_OK, dispense_cli_parse_portions("1", &portions));
    TEST_ASSERT_EQUAL_UINT8(1u, portions);
    TEST_ASSERT_EQUAL(PORT_OK, dispense_cli_parse_portions("15", &portions));
    TEST_ASSERT_EQUAL_UINT8(15u, portions);
}

void test_dispense_cli_parse_portions_rejects_invalid(void)
{
    uint8_t portions;

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_portions("0", &portions));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_portions("16", &portions));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_portions("01", &portions));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_portions("abc", &portions));
}

void test_dispense_cli_parse_grams_valid_range(void)
{
    uint8_t grams = 0u;

    TEST_ASSERT_EQUAL(PORT_OK, dispense_cli_parse_grams("5", &grams));
    TEST_ASSERT_EQUAL_UINT8(5u, grams);
    TEST_ASSERT_EQUAL(PORT_OK, dispense_cli_parse_grams("150", &grams));
    TEST_ASSERT_EQUAL_UINT8(150u, grams);
}

void test_dispense_cli_parse_grams_rejects_invalid(void)
{
    uint8_t grams = 0u;

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_grams("4", &grams));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_grams("151", &grams));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, dispense_cli_parse_grams("01", &grams));
}

void test_dispense_compensated_retries_until_tolerance_met(void)
{
    app_event_t ev;
    const fake_mqtt_port_state_t *mqtt;

    dispense_test_reset_all();
    dispense_test_setup_mqtt();
    TEST_ASSERT_TRUE(feed_config_mode_set(DISPENSE_MODE_COMPENSATED));
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    dispense_test_seed_baseline(100, 0u);
    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(100));
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK,
                      dispense_submit_grams(30u, DISPENSE_SOURCE_SCHEDULE));
    TEST_ASSERT_TRUE(app_step());

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    TEST_ASSERT_TRUE(dispense_is_active());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());

    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(120));
    {
        uint32_t settle_base = 10000u;

        dispense_poll(settle_base);
        dispense_poll(settle_base + DISPENSE_SETTLE_MS);
    }
    TEST_ASSERT_TRUE(dispense_is_active());
    TEST_ASSERT_EQUAL(2u, fake_motor_port_burst_calls());

    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(128));
    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    dispense_test_advance_settle(5000u);
    TEST_ASSERT_FALSE(dispense_is_active());
    TEST_ASSERT_EQUAL(2u, fake_motor_port_burst_calls());

    dispense_test_drain_mqtt();
    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"event_type\":\"success\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"grams\":28"));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"mode\":\"compensated\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"batch_count\":2"));
}

void test_dispense_compensated_underfill_publishes_event(void)
{
    const fake_mqtt_port_state_t *mqtt;
    uint32_t t = 10000u;

    dispense_test_reset_all();
    dispense_test_setup_mqtt();
    TEST_ASSERT_TRUE(feed_config_mode_set(DISPENSE_MODE_COMPENSATED));
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    dispense_test_seed_baseline(100, 0u);
    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(100));
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK,
                      dispense_submit_grams(30u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());

    dispense_test_run_burst_and_settle(110, &t);
    TEST_ASSERT_TRUE(dispense_is_active());
    dispense_test_run_burst_and_settle(115, &t);
    TEST_ASSERT_TRUE(dispense_is_active());
    dispense_test_run_burst_and_settle(118, &t);
    TEST_ASSERT_FALSE(dispense_is_active());
    TEST_ASSERT_EQUAL(3u, fake_motor_port_burst_calls());

    dispense_test_drain_mqtt();
    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"event_type\":\"underfill\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"grams\":18"));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"batch_count\":3"));
    TEST_ASSERT_NULL(strstr(mqtt->last_publish_payload, "deficit_g"));
}

void test_dispense_compensation_retry_motor_busy_aborts(void)
{
    app_event_t ev;
    const fake_mqtt_port_state_t *mqtt;

    dispense_test_reset_all();
    dispense_test_setup_mqtt();
    TEST_ASSERT_TRUE(feed_config_mode_set(DISPENSE_MODE_COMPENSATED));
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    dispense_test_seed_baseline(100, 0u);
    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(100));
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK,
                      dispense_submit_grams(30u, DISPENSE_SOURCE_MQTT));
    TEST_ASSERT_TRUE(app_step());

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());

    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(110));
    fake_motor_port_set_burst_err(PORT_ERR_IO);
    dispense_test_advance_settle(10000u);
    TEST_ASSERT_FALSE(dispense_is_active());

    dispense_test_drain_mqtt();
    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"event_type\":\"aborted\""));
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, "\"grams\":10"));
    TEST_ASSERT_NULL(strstr(mqtt->last_publish_payload, "deficit_g"));
}

void test_dispense_uart_portions_stays_open_loop_when_compensated(void)
{
    app_event_t ev;

    dispense_test_reset_all();
    TEST_ASSERT_TRUE(feed_config_mode_set(DISPENSE_MODE_COMPENSATED));
    dispense_test_seed_baseline(100, 0u);
    fake_weight_port_set_read_dg(WEIGHT_G_TO_DG(105));
    TEST_ASSERT_EQUAL(DISPENSE_SUBMIT_OK,
                      dispense_submit_portions(1u, DISPENSE_SOURCE_UART));
    TEST_ASSERT_TRUE(app_step());

    ev.type = EVT_BURST_DONE;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_step());
    dispense_test_advance_settle(1000u);

    TEST_ASSERT_FALSE(dispense_is_active());
    TEST_ASSERT_EQUAL(1u, fake_motor_port_burst_calls());
}
