/*
 * Host-side weigh calibration — spec/30-processes/weighing.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config_keys.h"
#include "weigh_cal.h"

static port_err_t write_i32(const config_port_t *cfg,
                            const char *key,
                            int32_t value)
{
    char buf[16];

    if (cfg == NULL || cfg->write == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    snprintf(buf, sizeof(buf), "%ld", (long)value);
    return cfg->write(CONFIG_GROUP_CALIB, key, buf);
}

static port_err_t read_i32(const config_port_t *cfg,
                           const char *key,
                           int32_t *out)
{
    char buf[16];
    char *end;

    if (cfg == NULL || cfg->read == NULL || out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (cfg->read(CONFIG_GROUP_CALIB, key, buf, sizeof(buf)) != PORT_OK) {
        return PORT_ERR_NOT_FOUND;
    }

    *out = (int32_t)strtol(buf, &end, 10);
    if (end == buf) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static port_err_t erase_key(const config_port_t *cfg, const char *key)
{
    if (cfg == NULL || cfg->erase == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    return cfg->erase(CONFIG_GROUP_CALIB, key);
}

port_err_t weigh_cal_load(const config_port_t *cfg, weigh_cal_model_t *out)
{
    port_err_t err;

    if (out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    if (cfg == NULL) {
        return PORT_OK;
    }

    err = read_i32(cfg, CONFIG_KEY_CALIB_ZERO, &out->zero_raw);
    if (err == PORT_OK) {
        out->zero_set = true;
    } else if (err != PORT_ERR_NOT_FOUND) {
        return err;
    }

    err = read_i32(cfg, CONFIG_KEY_CALIB_SPAN_G, &out->span_g);
    if (err == PORT_OK) {
        out->span_set = true;
    } else if (err != PORT_ERR_NOT_FOUND) {
        return err;
    }

    if (out->span_set) {
        err = read_i32(cfg, CONFIG_KEY_CALIB_SPAN_RAW, &out->span_raw);
        if (err != PORT_OK) {
            out->span_set = false;
            return err == PORT_ERR_NOT_FOUND ? PORT_ERR_IO : err;
        }
    }

    if (out->zero_set && !out->span_set) {
        return PORT_OK;
    }

    if (out->span_set && !out->zero_set) {
        out->span_set = false;
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

port_err_t weigh_cal_save_zero(const config_port_t *cfg,
                               int32_t zero_raw,
                               weigh_cal_model_t *model)
{
    port_err_t err;

    if (model == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (cfg != NULL) {
        err = write_i32(cfg, CONFIG_KEY_CALIB_ZERO, zero_raw);
        if (err != PORT_OK) {
            return err;
        }

        (void)erase_key(cfg, CONFIG_KEY_CALIB_SPAN_G);
        (void)erase_key(cfg, CONFIG_KEY_CALIB_SPAN_RAW);
    }

    model->zero_raw = zero_raw;
    model->zero_set = true;
    model->span_g = 0;
    model->span_raw = 0;
    model->span_set = false;
    return PORT_OK;
}

port_err_t weigh_cal_save_span(const config_port_t *cfg,
                               int32_t span_g,
                               int32_t span_raw,
                               weigh_cal_model_t *model)
{
    port_err_t err;

    if (model == NULL || !model->zero_set) {
        return PORT_ERR_INVALID_ARG;
    }

    if (span_g != WEIGH_CAL_SPAN_G) {
        return PORT_ERR_INVALID_ARG;
    }

    if (span_raw == model->zero_raw) {
        return PORT_ERR_INVALID_ARG;
    }

    if (cfg != NULL) {
        err = write_i32(cfg, CONFIG_KEY_CALIB_SPAN_G, span_g);
        if (err != PORT_OK) {
            return err;
        }

        err = write_i32(cfg, CONFIG_KEY_CALIB_SPAN_RAW, span_raw);
        if (err != PORT_OK) {
            return err;
        }
    }

    model->span_g = span_g;
    model->span_raw = span_raw;
    model->span_set = true;
    return PORT_OK;
}

port_err_t weigh_cal_clear(const config_port_t *cfg, weigh_cal_model_t *model)
{
    if (model == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (cfg != NULL) {
        (void)erase_key(cfg, CONFIG_KEY_CALIB_ZERO);
        (void)erase_key(cfg, CONFIG_KEY_CALIB_SPAN_G);
        (void)erase_key(cfg, CONFIG_KEY_CALIB_SPAN_RAW);
    }

    memset(model, 0, sizeof(*model));
    return PORT_OK;
}

bool weigh_cal_is_complete(const weigh_cal_model_t *model)
{
    return model != NULL && model->zero_set && model->span_set;
}

bool weigh_cal_zero_pending_span(const weigh_cal_model_t *model)
{
    return model != NULL && model->zero_set && !model->span_set;
}

port_err_t weigh_cal_apply(const weigh_cal_model_t *model, int32_t raw, int32_t *grams)
{
    int64_t num;
    int64_t den;

    if (model == NULL || grams == NULL || !weigh_cal_is_complete(model)) {
        return PORT_ERR_INVALID_ARG;
    }

    den = (int64_t)model->span_raw - (int64_t)model->zero_raw;
    if (den == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    num = (int64_t)(raw - model->zero_raw) * (int64_t)model->span_g;
    *grams = (int32_t)(num / den);
    return PORT_OK;
}

port_err_t weigh_cal_food_grams(const weigh_cal_model_t *model, int32_t raw, int32_t *food_g)
{
    int32_t total;
    port_err_t err;

    if (food_g == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = weigh_cal_apply(model, raw, &total);
    if (err != PORT_OK) {
        return err;
    }

    *food_g = total - WEIGH_BOWL_MASS_G;
    return PORT_OK;
}
