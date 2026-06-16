/*
 * Dispense supervisor — spec/30-processes/dispense-cycle.md
 */

#include "dispense.h"

#include "app.h"
#include "app_event.h"
#include "app_log.h"
#include "dispense_cli.h"
#include "display_dispense_indicator.h"
#include "hopper_input.h"
#include "motor_jam.h"
#include "motor_port.h"
#include "port_err.h"

static bool s_job_active;
static bool s_job_pending;

static void dispense_finish_job(dispense_outcome_t outcome)
{
    if (!s_job_active && !s_job_pending) {
        return;
    }

    s_job_active = false;
    s_job_pending = false;
    display_dispense_indicator_idle();

    if (outcome == DISPENSE_OUTCOME_SUCCESS) {
        (void)dispense_cli_on_job_done();
        hopper_input_notify_dispense_complete();
        app_weight_notify_dispense_complete();
    } else {
        (void)dispense_cli_on_job_fault();
        hopper_input_notify_dispense_complete();
    }
}

static port_err_t dispense_kick_motor(uint8_t pulse_target)
{
    const motor_port_t *motor = motor_port_get();

    if (motor == NULL || motor->request_burst == NULL) {
        return PORT_ERR_IO;
    }

    return motor->request_burst(pulse_target, MOTOR_BURST_TIMEOUT_MS);
}

dispense_submit_result_t dispense_submit_portions(uint8_t portions)
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

    if (!app_event_post(&ev)) {
        app_log_info("dispense", "busy");
        return DISPENSE_SUBMIT_BUSY;
    }

    s_job_pending = true;
    app_log_info("dispense", "started portions=%u", (unsigned)portions);
    return DISPENSE_SUBMIT_OK;
}

bool dispense_is_active(void)
{
    return s_job_active || s_job_pending;
}

void dispense_start_from_request(const app_dispense_request_t *req)
{
    uint8_t portions;
    port_err_t err;

    if (req == NULL || !s_job_pending || s_job_active) {
        app_log_info("dispense", "busy");
        dispense_cli_cancel_wait();
        s_job_pending = false;
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
    s_job_active = true;
    display_dispense_indicator_active();

    err = dispense_kick_motor(portions);
    if (err != PORT_OK) {
        s_job_active = false;
        display_dispense_indicator_idle();
        app_log_info("dispense", "busy");
        dispense_cli_cancel_wait();
    }
}

bool dispense_on_burst_done(void)
{
    if (!s_job_active) {
        return false;
    }

    dispense_finish_job(DISPENSE_OUTCOME_SUCCESS);
    return true;
}

bool dispense_on_motor_fault(void)
{
    if (!s_job_active) {
        return false;
    }

    dispense_finish_job(DISPENSE_OUTCOME_STUCK);
    return true;
}

void dispense_poll(void)
{
    const motor_port_t *motor;

    if (!s_job_active) {
        return;
    }

    motor = motor_port_get();
    if (motor == NULL || motor->is_active == NULL) {
        return;
    }

    if (!motor->is_active()) {
        dispense_finish_job(DISPENSE_OUTCOME_SUCCESS);
    }
}

void dispense_test_reset(void)
{
    s_job_active = false;
    s_job_pending = false;
    display_dispense_indicator_idle();
}
