/*
 * Host-side weigh calibration — spec/30-processes/weighing.md
 */

#ifndef WEIGH_CAL_H
#define WEIGH_CAL_H

#include <stdbool.h>
#include <stdint.h>

#include "config_port.h"
#include "port_err.h"
#include "weight_units.h"

#include "weigh_product.h"

#define WEIGH_CAL_SPAN_G  WEIGH_BOWL_MASS_G

typedef struct weigh_cal_model {
    int32_t zero_raw;
    int32_t span_g;
    int32_t span_raw;
    bool zero_set;
    bool span_set;
} weigh_cal_model_t;

port_err_t weigh_cal_load(const config_port_t *cfg, weigh_cal_model_t *out);
port_err_t weigh_cal_save_zero(const config_port_t *cfg,
                               int32_t zero_raw,
                               weigh_cal_model_t *model);
port_err_t weigh_cal_save_span(const config_port_t *cfg,
                               int32_t span_g,
                               int32_t span_raw,
                               weigh_cal_model_t *model);
port_err_t weigh_cal_clear(const config_port_t *cfg, weigh_cal_model_t *model);
bool weigh_cal_is_complete(const weigh_cal_model_t *model);
bool weigh_cal_zero_pending_span(const weigh_cal_model_t *model);
port_err_t weigh_cal_apply_dg(const weigh_cal_model_t *model, int32_t raw, weight_dg_t *total_dg);
port_err_t weigh_cal_food_dg(const weigh_cal_model_t *model, int32_t raw, weight_dg_t *food_dg);

#endif /* WEIGH_CAL_H */
