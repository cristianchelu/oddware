/*
 * Auto-tare drift compensation — spec/30-processes/auto-tare.md
 */

#ifndef AUTO_TARE_H
#define AUTO_TARE_H

#include "bowl_error.h"
#include "weight_units.h"

#include <stdbool.h>
#include <stdint.h>

#define AUTO_TARE_DRIFT_RATE_MAX_DG       1
#define AUTO_TARE_INITIAL_STABLE_STREAK   4u

void auto_tare_init(void);
void auto_tare_on_bowl_removed(void);
void auto_tare_on_bowl_present(void);
void auto_tare_sync_bowl_error(bowl_error_kind_t bowl_err);
void auto_tare_idle_sample(weight_dg_t raw_dg, bool sample_valid);
void auto_tare_anchor(weight_dg_t raw_dg);
weight_dg_t auto_tare_present_dg(weight_dg_t raw_dg, bool sample_valid);

bool auto_tare_pending_calibration(void);
bool auto_tare_stable_valid(void);
weight_dg_t auto_tare_stable_dg(void);
weight_dg_t auto_tare_drift_offset_dg(void);

void auto_tare_test_reset(void);

#endif /* AUTO_TARE_H */
