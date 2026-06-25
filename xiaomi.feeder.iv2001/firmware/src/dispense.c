/*
 * Dispense supervisor — spec/30-processes/dispense-cycle.md
 */

#include "dispense.h"

#include "app.h"
#include "app_event.h"
#include "app_log.h"
#include "bowl_error.h"
#include "dispense_cli.h"
#include "display_dispense_indicator.h"
#include "feed_config.h"
#include "FreeRTOS.h"
#include "hopper_input.h"
#include "hopper_level.h"
#include "motor_jam.h"
#include "motor_port.h"
#include "mqtt_client.h"
#include "mqtt_dispense_event.h"
#include "port_err.h"
#include "schedule.h"
#include "task.h"
#include "weight_port.h"

typedef enum {
    DISPENSE_PHASE_NONE = 0,
    DISPENSE_PHASE_MOTOR,
    DISPENSE_PHASE_SETTLE,
} dispense_phase_t;

static bool s_job_pending;
static dispense_phase_t s_phase;
static dispense_outcome_t s_outcome;
static dispense_source_t s_source;
static uint8_t s_portions;
static uint8_t s_total_portions;
static uint16_t s_target_grams;
static bool s_gram_job;
static dispense_mode_t s_mode;
static uint8_t s_batch_count;
static int32_t s_baseline_grams;
static int32_t s_last_settle_grams;
static bool s_baseline_valid;
static uint32_t s_settle_start_ms;
static uint8_t s_zero_delta_streak;

static bool dispense_capture_baseline(uint32_t now_ms)
{
    app_bowl_grams_snapshot_t snap;

    if (app_bowl_grams_snapshot(now_ms, &snap) &&
        snap.valid &&
        snap.sample_age_ms < DISPENSE_BASELINE_FRESH_MS) {
        s_baseline_grams = snap.grams;
        s_baseline_valid = true;
        return true;
    }

    {
        const weight_port_t *wp = weight_port_get();
        int32_t grams;
        port_err_t err;

        if (wp == NULL || wp->read_grams == NULL) {
            s_baseline_valid = false;
            return false;
        }

        err = wp->read_grams(&grams);
        if (err != PORT_OK) {
            s_baseline_valid = false;
            return false;
        }

        s_baseline_grams = grams;
        s_baseline_valid = true;
        app_bowl_grams_notify_read(grams, true, now_ms);
        return true;
    }
}

static bool dispense_read_post_grams(int32_t *grams_out)
{
    const weight_port_t *wp = weight_port_get();
    port_err_t err;

    if (wp == NULL || wp->read_grams == NULL || grams_out == NULL) {
        return false;
    }

    err = wp->read_grams(grams_out);
    return err == PORT_OK;
}

static bool dispense_scale_trusted(int32_t grams, bool valid)
{
    const weight_port_t *wp = weight_port_get();
    weight_cal_status_t cal;
    bowl_error_kind_t bowl_err;

    if (!valid) {
        return false;
    }

    cal = (wp != NULL && wp->get_cal_status != NULL) ? wp->get_cal_status()
                                                   : WEIGHT_CAL_UNCALIBRATED;
    bowl_err = bowl_error_eval(cal, true, grams);
    return !bowl_error_is_active(bowl_err);
}

static void dispense_update_zero_delta_streak(int32_t raw_delta)
{
    if (raw_delta <= 0) {
        if (s_zero_delta_streak < 255u) {
            s_zero_delta_streak++;
        }
    } else {
        s_zero_delta_streak = 0u;
    }
}

static dispense_mode_t dispense_resolve_mode(const app_dispense_request_t *req)
{
    if (req != NULL &&
        req->kind == DISPENSE_KIND_PORTIONS &&
        req->source == DISPENSE_SOURCE_UART) {
        return DISPENSE_MODE_OPEN_LOOP;
    }

    return feed_config_mode_get();
}

static void dispense_notify_cli_outcome(dispense_outcome_t outcome)
{
    if (outcome == DISPENSE_OUTCOME_SUCCESS) {
        (void)dispense_cli_on_job_done();
    } else {
        (void)dispense_cli_on_job_fault(outcome);
    }
}

static void dispense_publish_terminal(uint32_t now_ms,
                                      int32_t event_grams,
                                      bool measured,
                                      int32_t raw_delta)
{
    dispense_completion_t completion;

    s_outcome = hopper_level_on_dispense_finished(s_outcome,
                                                  raw_delta,
                                                  measured,
                                                  now_ms);

    completion.grams = event_grams;
    completion.grams_estimated = !measured;
    completion.target_g = s_target_grams;
    completion.outcome = s_outcome;
    completion.source = s_source;
    completion.mode = s_mode;
    completion.batch_count = s_batch_count;
    completion.portions = s_total_portions;
    completion.has_slot = schedule_active_slot(&completion.slot_hour, &completion.slot_min);

    if (s_source == DISPENSE_SOURCE_SCHEDULE && completion.has_slot) {
        schedule_dispense_result_t sched_result;

        sched_result.hour = completion.slot_hour;
        sched_result.min = completion.slot_min;
        sched_result.grams = completion.grams;

        switch (s_outcome) {
        case DISPENSE_OUTCOME_SUCCESS:
            sched_result.outcome = SCHEDULE_DISPENSE_OK;
            break;
        case DISPENSE_OUTCOME_UNDERFILL:
            sched_result.outcome = SCHEDULE_DISPENSE_UNDERFILL;
            break;
        default:
            sched_result.outcome = SCHEDULE_DISPENSE_FAILED;
            break;
        }

        schedule_on_dispense_complete(&sched_result);
    }

    {
        const char *device_id = mqtt_client_device_id();

        if (device_id != NULL && device_id[0] != '\0') {
            mqtt_dispense_event_set_device_id(device_id);
            (void)mqtt_dispense_event_publish(&completion);
        }
    }

    app_weight_notify_dispense_complete();

    s_phase = DISPENSE_PHASE_NONE;
    s_job_pending = false;
    s_gram_job = false;
    display_dispense_indicator_idle();

    dispense_notify_cli_outcome(s_outcome);
    hopper_level_notify_dispense_complete();
}

static void dispense_begin_settle(dispense_outcome_t outcome)
{
    s_outcome = outcome;
    s_phase = DISPENSE_PHASE_SETTLE;
    s_settle_start_ms = 0u;
}

static port_err_t dispense_kick_motor(uint8_t pulse_target)
{
    const motor_port_t *motor = motor_port_get();

    if (motor == NULL || motor->request_burst == NULL) {
        return PORT_ERR_IO;
    }

    return motor->request_burst(pulse_target, MOTOR_BURST_TIMEOUT_MS);
}

static void dispense_after_settle(uint32_t now_ms)
{
    int32_t post_grams = 0;
    bool post_valid;
    int32_t raw_delta;
    int32_t batch_delta;
    int32_t grams_delivered;
    int32_t event_grams;
    bool measured;

    if (s_phase == DISPENSE_PHASE_NONE) {
        return;
    }

    post_valid = dispense_read_post_grams(&post_grams);
    if (post_valid) {
        app_bowl_grams_notify_read(post_grams, true, now_ms);
    }

    measured = s_baseline_valid && post_valid &&
               dispense_scale_trusted(s_baseline_grams, s_baseline_valid) &&
               dispense_scale_trusted(post_grams, post_valid);

    if (measured) {
        grams_delivered = post_grams - s_baseline_grams;
        batch_delta = post_grams - s_last_settle_grams;
        dispense_update_zero_delta_streak(batch_delta);
        s_last_settle_grams = post_grams;
        raw_delta = grams_delivered;
        event_grams = grams_delivered;
        if (event_grams < 0) {
            app_log_info("dispense", "negative delta clamped raw=%ld", (long)raw_delta);
            event_grams = 0;
        }
    } else {
        if (s_baseline_valid && post_valid) {
            raw_delta = post_grams - s_baseline_grams;
            batch_delta = post_grams - s_last_settle_grams;
            dispense_update_zero_delta_streak(batch_delta);
            s_last_settle_grams = post_grams;
        } else {
            raw_delta = 0;
            batch_delta = 0;
        }
        grams_delivered = (int32_t)s_total_portions * (int32_t)DISPENSE_GRAMS_PER_PORTION;
        event_grams = grams_delivered;
    }

    if (s_outcome == DISPENSE_OUTCOME_STUCK) {
        dispense_publish_terminal(now_ms, event_grams, measured, raw_delta);
        return;
    }

    if (s_mode == DISPENSE_MODE_OPEN_LOOP || !measured) {
        dispense_publish_terminal(now_ms, event_grams, measured, raw_delta);
        return;
    }

    if (grams_delivered >= (int32_t)s_target_grams) {
        s_outcome = DISPENSE_OUTCOME_SUCCESS;
        dispense_publish_terminal(now_ms, event_grams, measured, raw_delta);
        return;
    }

    if (grams_delivered >= (int32_t)s_target_grams - (int32_t)DISPENSE_COMP_TOLERANCE_G) {
        s_outcome = DISPENSE_OUTCOME_SUCCESS;
        dispense_publish_terminal(now_ms, event_grams, measured, raw_delta);
        return;
    }

    if (s_batch_count >= DISPENSE_COMP_MAX_BATCHES) {
        s_outcome = DISPENSE_OUTCOME_UNDERFILL;
        dispense_publish_terminal(now_ms, event_grams, measured, raw_delta);
        return;
    }

    if (s_zero_delta_streak >= DISPENSE_COMP_ZERO_DELTA_GIVEUP) {
        s_outcome = DISPENSE_OUTCOME_UNDERFILL;
        dispense_publish_terminal(now_ms, event_grams, measured, raw_delta);
        return;
    }

    {
        int32_t deficit = (int32_t)s_target_grams - grams_delivered;
        uint8_t extra;
        port_err_t err;

        extra = (uint8_t)(deficit / (int32_t)DISPENSE_GRAMS_PER_PORTION);
        if (extra < 1u) {
            extra = 1u;
        }
        if (extra > DISPENSE_PORTIONS_MAX) {
            extra = DISPENSE_PORTIONS_MAX;
        }

        err = dispense_kick_motor(extra);
        if (err != PORT_OK) {
            s_outcome = DISPENSE_OUTCOME_ABORTED;
            dispense_publish_terminal(now_ms, event_grams, measured, raw_delta);
            return;
        }

        s_total_portions = (uint8_t)(s_total_portions + extra);
        s_batch_count++;
        s_phase = DISPENSE_PHASE_MOTOR;
        s_settle_start_ms = 0u;
    }
}

dispense_submit_result_t dispense_submit_portions(uint8_t portions,
                                                  dispense_source_t source)
{
    app_event_t ev;

    if (portions < DISPENSE_PORTIONS_MIN || portions > DISPENSE_PORTIONS_MAX) {
        app_log_info("dispense", "rejected result=%d", (int)DISPENSE_SUBMIT_INVALID);
        return DISPENSE_SUBMIT_INVALID;
    }

    if (dispense_is_active()) {
        app_log_info("dispense", "busy");
        return DISPENSE_SUBMIT_BUSY;
    }

    {
        const motor_port_t *motor = motor_port_get();

        if (motor != NULL && motor->is_active != NULL && motor->is_active()) {
            app_log_info("dispense", "busy");
            return DISPENSE_SUBMIT_BUSY;
        }
    }

    ev.type = EVT_DISPENSE_REQUEST;
    ev.u.dispense_request.kind = DISPENSE_KIND_PORTIONS;
    ev.u.dispense_request.target = portions;
    ev.u.dispense_request.source = source;

    if (!app_event_post(&ev)) {
        app_log_info("dispense", "busy");
        return DISPENSE_SUBMIT_BUSY;
    }

    s_job_pending = true;
    app_log_info("dispense", "started portions=%u", (unsigned)portions);
    return DISPENSE_SUBMIT_OK;
}

dispense_submit_result_t dispense_submit_grams(uint8_t grams,
                                               dispense_source_t source)
{
    app_event_t ev;

    if (grams < SCHEDULE_G_MIN || grams > SCHEDULE_G_MAX) {
        app_log_info("dispense", "rejected result=%d", (int)DISPENSE_SUBMIT_INVALID);
        return DISPENSE_SUBMIT_INVALID;
    }

    if (dispense_is_active()) {
        app_log_info("dispense", "busy");
        return DISPENSE_SUBMIT_BUSY;
    }

    {
        const motor_port_t *motor = motor_port_get();

        if (motor != NULL && motor->is_active != NULL && motor->is_active()) {
            app_log_info("dispense", "busy");
            return DISPENSE_SUBMIT_BUSY;
        }
    }

    ev.type = EVT_DISPENSE_REQUEST;
    ev.u.dispense_request.kind = DISPENSE_KIND_GRAMS;
    ev.u.dispense_request.target = grams;
    ev.u.dispense_request.source = source;

    if (!app_event_post(&ev)) {
        app_log_info("dispense", "busy");
        return DISPENSE_SUBMIT_BUSY;
    }

    s_job_pending = true;
    app_log_info("dispense", "started grams=%u", (unsigned)grams);
    return DISPENSE_SUBMIT_OK;
}

bool dispense_is_active(void)
{
    return s_job_pending || s_phase != DISPENSE_PHASE_NONE;
}

void dispense_start_from_request(const app_dispense_request_t *req)
{
    uint8_t portions;
    port_err_t err;
    uint32_t now_ms;

    if (req == NULL || !s_job_pending || s_phase != DISPENSE_PHASE_NONE) {
        app_log_info("dispense", "busy");
        dispense_cli_cancel_wait();
        s_job_pending = false;
        return;
    }

    if (req->kind == DISPENSE_KIND_GRAMS) {
        uint8_t grams = (uint8_t)req->target;
        uint8_t portions;

        if (grams < SCHEDULE_G_MIN || grams > SCHEDULE_G_MAX) {
            app_log_info("dispense", "busy");
            dispense_cli_cancel_wait();
            s_job_pending = false;
            return;
        }

        portions = (uint8_t)((grams + DISPENSE_GRAMS_PER_PORTION - 1u) /
                             DISPENSE_GRAMS_PER_PORTION);
        if (portions < DISPENSE_PORTIONS_MIN) {
            portions = DISPENSE_PORTIONS_MIN;
        }
        if (portions > DISPENSE_PORTIONS_MAX) {
            portions = DISPENSE_PORTIONS_MAX;
        }

        s_target_grams = grams;
        s_gram_job = true;
        s_portions = portions;
        s_total_portions = portions;
        s_source = req->source;
        s_mode = dispense_resolve_mode(req);
        s_batch_count = 1u;
        s_zero_delta_streak = 0u;
        s_job_pending = false;
        s_phase = DISPENSE_PHASE_MOTOR;

        now_ms = (uint32_t)(xTaskGetTickCount() * (TickType_t)portTICK_PERIOD_MS);
        (void)dispense_capture_baseline(now_ms);
        s_last_settle_grams = s_baseline_valid ? s_baseline_grams : 0;

        if (display_dispense_indicator_active() != PORT_OK) {
            app_log_info("dispense", "indicator unavailable");
        }

        err = dispense_kick_motor(portions);
        if (err != PORT_OK) {
            s_phase = DISPENSE_PHASE_NONE;
            s_gram_job = false;
            display_dispense_indicator_idle();
            app_log_info("dispense", "busy");
            dispense_cli_cancel_wait();
            return;
        }

        return;
    }

    if (req->kind != DISPENSE_KIND_PORTIONS) {
        app_log_info("dispense", "busy");
        dispense_cli_cancel_wait();
        s_job_pending = false;
        return;
    }

    portions = (uint8_t)req->target;
    if (portions < DISPENSE_PORTIONS_MIN || portions > DISPENSE_PORTIONS_MAX) {
        app_log_info("dispense", "busy");
        dispense_cli_cancel_wait();
        s_job_pending = false;
        return;
    }

    s_job_pending = false;
    s_gram_job = false;
    s_portions = portions;
    s_total_portions = portions;
    s_target_grams = (uint16_t)portions * DISPENSE_GRAMS_PER_PORTION;
    s_source = req->source;
    s_mode = dispense_resolve_mode(req);
    s_batch_count = 1u;
    s_zero_delta_streak = 0u;
    s_phase = DISPENSE_PHASE_MOTOR;

    now_ms = (uint32_t)(xTaskGetTickCount() * (TickType_t)portTICK_PERIOD_MS);
    (void)dispense_capture_baseline(now_ms);
    s_last_settle_grams = s_baseline_valid ? s_baseline_grams : 0;

    if (display_dispense_indicator_active() != PORT_OK) {
        app_log_info("dispense", "indicator unavailable");
    }

    err = dispense_kick_motor(portions);
    if (err != PORT_OK) {
        s_phase = DISPENSE_PHASE_NONE;
        display_dispense_indicator_idle();
        app_log_info("dispense", "busy");
        dispense_cli_cancel_wait();
    }
}

bool dispense_on_burst_done(void)
{
    if (s_phase != DISPENSE_PHASE_MOTOR) {
        return false;
    }

    dispense_begin_settle(DISPENSE_OUTCOME_SUCCESS);
    return true;
}

bool dispense_on_motor_fault(void)
{
    if (s_phase != DISPENSE_PHASE_MOTOR) {
        return false;
    }

    dispense_begin_settle(DISPENSE_OUTCOME_STUCK);
    return true;
}

void dispense_poll(uint32_t now_ms)
{
    if (s_phase != DISPENSE_PHASE_SETTLE) {
        return;
    }

    if (s_settle_start_ms == 0u) {
        s_settle_start_ms = now_ms;
        return;
    }

    if ((now_ms - s_settle_start_ms) < DISPENSE_SETTLE_MS) {
        return;
    }

    dispense_after_settle(now_ms);
}

uint8_t dispense_test_zero_delta_streak(void)
{
    return s_zero_delta_streak;
}

void dispense_test_reset(void)
{
    s_job_pending = false;
    s_phase = DISPENSE_PHASE_NONE;
    s_outcome = DISPENSE_OUTCOME_SUCCESS;
    s_source = DISPENSE_SOURCE_MQTT;
    s_portions = 0u;
    s_total_portions = 0u;
    s_gram_job = false;
    s_mode = DISPENSE_MODE_OPEN_LOOP;
    s_batch_count = 0u;
    s_target_grams = 0u;
    s_baseline_grams = 0;
    s_last_settle_grams = 0;
    s_baseline_valid = false;
    s_settle_start_ms = 0u;
    s_zero_delta_streak = 0u;
    display_dispense_indicator_idle();
}
