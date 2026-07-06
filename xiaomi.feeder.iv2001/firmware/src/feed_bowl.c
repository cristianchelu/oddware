/*
 * Bowl mass helpers for feed policy — spec/30-processes/scheduler-engine.md § Overfill
 */

#include "feed_bowl.h"

#include "app.h"
#include "bowl_mass_present.h"
#include "dispense.h"
#include "feed_config.h"
#include "weight_port.h"
#include "weight_units.h"

bool feed_bowl_known_g(uint32_t now_ms, uint16_t *grams_out)
{
    const weight_port_t *wp = weight_port_get();
    app_bowl_dg_snapshot_t snap;
    weight_cal_status_t cal = WEIGHT_CAL_UNCALIBRATED;
    weight_dg_t present_dg = 0;
    bowl_mass_status_t status;

    if (grams_out != NULL) {
        *grams_out = 0u;
    }

    if (wp != NULL && wp->get_cal_status != NULL) {
        cal = wp->get_cal_status();
    }

    if (!app_bowl_dg_snapshot(now_ms, &snap)) {
        return false;
    }

    status = bowl_mass_present_dg(cal, snap.valid, snap.dg, &present_dg);
    if (status != BOWL_MASS_KNOWN) {
        return false;
    }

    if (grams_out != NULL) {
        *grams_out = weight_dg_to_display_g(present_dg);
    }

    return true;
}

bool feed_overfill_should_skip_schedule(uint32_t now_ms)
{
    uint16_t bowl_g = 0u;

    if (!feed_config_overfill_enabled_get()) {
        return false;
    }

    if (!feed_bowl_known_g(now_ms, &bowl_g)) {
        return false;
    }

    return bowl_g >= feed_config_overfill_threshold_g_get();
}

schedule_fire_result_t feed_schedule_fire(uint8_t g, uint32_t now_ms)
{
    dispense_submit_result_t result;

    if (feed_overfill_should_skip_schedule(now_ms)) {
        return SCHEDULE_FIRE_SKIPPED_FULL;
    }

    result = dispense_submit_grams(g, DISPENSE_SOURCE_SCHEDULE);
    switch (result) {
    case DISPENSE_SUBMIT_OK:
        return SCHEDULE_FIRE_OK;
    case DISPENSE_SUBMIT_BUSY:
        return SCHEDULE_FIRE_BUSY;
    default:
        return SCHEDULE_FIRE_REJECTED;
    }
}
