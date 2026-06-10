#include "fake_weight_port.h"

#include <stdbool.h>
#include <string.h>

#define FAKE_WEIGHT_OP_MAX  16u

static fake_weight_op_t s_ops[FAKE_WEIGHT_OP_MAX];
static size_t s_op_count;
static int32_t s_read_grams = 100;
static port_err_t s_read_err = PORT_OK;
static weight_cal_status_t s_cal_status = WEIGHT_CAL_IDLE;
static bool s_scale_off;

static void record(fake_weight_op_kind_t kind)
{
    if (s_op_count >= FAKE_WEIGHT_OP_MAX) {
        return;
    }

    s_ops[s_op_count].kind = kind;
    s_ops[s_op_count].grams = s_read_grams;
    s_op_count++;
}

void fake_weight_port_reset(void)
{
    memset(s_ops, 0, sizeof(s_ops));
    s_op_count = 0u;
    s_read_grams = 100;
    s_read_err = PORT_OK;
    s_cal_status = WEIGHT_CAL_IDLE;
    s_scale_off = false;
}

void fake_weight_port_set_scale_off(bool off)
{
    s_scale_off = off;
}

void fake_weight_port_set_read_grams(int32_t grams)
{
    s_read_grams = grams;
}

void fake_weight_port_set_read_err(port_err_t err)
{
    s_read_err = err;
}

void fake_weight_port_set_cal_status(weight_cal_status_t st)
{
    s_cal_status = st;
}

const fake_weight_op_t *fake_weight_port_ops(size_t *count)
{
    if (count != NULL) {
        *count = s_op_count;
    }
    return s_ops;
}

static port_err_t fake_power_on(void)
{
    s_scale_off = false;
    record(FAKE_WEIGHT_OP_POWER_ON);
    return PORT_OK;
}

static port_err_t fake_power_off(void)
{
    s_scale_off = true;
    record(FAKE_WEIGHT_OP_POWER_OFF);
    return PORT_OK;
}

static port_err_t fake_read_grams(int32_t *grams)
{
    if (s_scale_off) {
        return PORT_ERR_NOT_FOUND;
    }

    record(FAKE_WEIGHT_OP_READ_GRAMS);
    if (s_read_err != PORT_OK) {
        return s_read_err;
    }
    if (grams != NULL) {
        *grams = s_read_grams;
    }
    return PORT_OK;
}

static port_err_t fake_read_raw_grams(int32_t *grams)
{
    return fake_read_grams(grams);
}

static port_err_t fake_cal_zero(void)
{
    record(FAKE_WEIGHT_OP_CAL_ZERO);
    s_cal_status = WEIGHT_CAL_CAPTURING_SPAN;
    return PORT_OK;
}

static port_err_t fake_cal_span(void)
{
    if (s_op_count < FAKE_WEIGHT_OP_MAX) {
        record(FAKE_WEIGHT_OP_CAL_SPAN);
    }
    s_cal_status = WEIGHT_CAL_SUCCESS;
    return PORT_OK;
}

static weight_cal_status_t fake_get_cal_status(void)
{
    return s_cal_status;
}

static const weight_port_t s_fake_weight_port = {
    .power_on = fake_power_on,
    .power_off = fake_power_off,
    .read_grams = fake_read_grams,
    .read_raw_grams = fake_read_raw_grams,
    .calibrate_zero = fake_cal_zero,
    .calibrate_span = fake_cal_span,
    .get_cal_status = fake_get_cal_status,
};

const weight_port_t *fake_weight_port_get(void)
{
    return &s_fake_weight_port;
}

const weight_port_t *weight_port_get(void)
{
    return fake_weight_port_get();
}
