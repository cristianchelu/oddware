/*
 * Auto-tare drift compensation — spec/30-processes/auto-tare.md
 */

#include "auto_tare.h"

#include "bowl_error.h"
#include "feeder_runtime.h"

#include <stddef.h>

static weight_dg_t s_stable_dg;
static weight_dg_t s_drift_offset_dg;
static bool s_stable_valid;
static bool s_pending_calibration;
static weight_dg_t s_prev_raw_dg;
static bool s_prev_raw_valid;
static uint8_t s_quiet_streak;
static bool s_bowl_err_sync_known;
static bowl_error_kind_t s_last_bowl_err_sync;

static weight_dg_t auto_tare_abs_delta(weight_dg_t a, weight_dg_t b)
{
    weight_dg_t d = a - b;

    return (d < 0) ? -d : d;
}

static bool auto_tare_sample_quiet(weight_dg_t raw_dg)
{
    if (!s_prev_raw_valid) {
        return true;
    }

    return auto_tare_abs_delta(raw_dg, s_prev_raw_dg) <= AUTO_TARE_DRIFT_RATE_MAX_DG;
}

static void auto_tare_track_raw(weight_dg_t raw_dg)
{
    s_prev_raw_dg = raw_dg;
    s_prev_raw_valid = true;
}

static void auto_tare_reset_tracking(void)
{
    s_prev_raw_valid = false;
    s_quiet_streak = 0u;
}

void auto_tare_init(void)
{
    s_stable_dg = 0;
    s_drift_offset_dg = 0;
    s_stable_valid = false;
    s_pending_calibration = true;
    s_bowl_err_sync_known = false;
    s_last_bowl_err_sync = BOWL_ERROR_NONE;
    auto_tare_reset_tracking();
}

void auto_tare_on_bowl_removed(void)
{
    s_stable_valid = false;
    s_pending_calibration = true;
    s_drift_offset_dg = 0;
    auto_tare_reset_tracking();
}

void auto_tare_on_bowl_present(void)
{
    auto_tare_reset_tracking();
}

void auto_tare_sync_bowl_error(bowl_error_kind_t bowl_err)
{
    bool is_active = bowl_error_is_active(bowl_err);

    if (!s_bowl_err_sync_known) {
        s_bowl_err_sync_known = true;
        s_last_bowl_err_sync = bowl_err;
        if (is_active) {
            auto_tare_on_bowl_removed();
        }
        return;
    }

    {
        bool was_active = bowl_error_is_active(s_last_bowl_err_sync);

        if (!was_active && is_active) {
            auto_tare_on_bowl_removed();
        } else if (was_active && !is_active) {
            auto_tare_on_bowl_present();
        }
    }

    s_last_bowl_err_sync = bowl_err;
}

void auto_tare_anchor(weight_dg_t raw_dg)
{
    s_stable_dg = raw_dg;
    s_drift_offset_dg = 0;
    s_stable_valid = true;
    s_pending_calibration = false;
    auto_tare_reset_tracking();
}

void auto_tare_idle_sample(weight_dg_t raw_dg, bool sample_valid)
{
    if (feeder_runtime_dispense_active()) {
        return;
    }

    if (!sample_valid) {
        auto_tare_reset_tracking();
        return;
    }

    if (s_pending_calibration) {
        if (auto_tare_sample_quiet(raw_dg)) {
            if (s_quiet_streak < UINT8_MAX) {
                s_quiet_streak++;
            }
            if (s_quiet_streak >= AUTO_TARE_INITIAL_STABLE_STREAK) {
                auto_tare_anchor(raw_dg);
                return;
            }
        } else {
            s_quiet_streak = 0u;
        }

        auto_tare_track_raw(raw_dg);
        return;
    }

    if (!s_stable_valid) {
        auto_tare_track_raw(raw_dg);
        return;
    }

    if (s_prev_raw_valid && auto_tare_sample_quiet(raw_dg)) {
        s_drift_offset_dg = s_stable_dg - raw_dg;
    }

    auto_tare_track_raw(raw_dg);
}

weight_dg_t auto_tare_present_dg(weight_dg_t raw_dg, bool sample_valid)
{
    if (!sample_valid || !s_stable_valid || s_pending_calibration) {
        return raw_dg;
    }

    return raw_dg + s_drift_offset_dg;
}

bool auto_tare_pending_calibration(void)
{
    return s_pending_calibration;
}

bool auto_tare_stable_valid(void)
{
    return s_stable_valid;
}

weight_dg_t auto_tare_stable_dg(void)
{
    return s_stable_dg;
}

weight_dg_t auto_tare_drift_offset_dg(void)
{
    return s_drift_offset_dg;
}

void auto_tare_test_reset(void)
{
    feeder_runtime_test_reset();
    auto_tare_init();
}
