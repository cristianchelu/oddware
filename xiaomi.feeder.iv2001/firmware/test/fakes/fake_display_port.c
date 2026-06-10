#include <string.h>

#include "fake_display_port.h"

#define FAKE_DISPLAY_MAX_OPS 16u

static fake_display_op_t s_ops[FAKE_DISPLAY_MAX_OPS];
static size_t s_op_count;
static port_err_t s_power_on_err = PORT_OK;
static port_err_t s_show_fill_err = PORT_OK;
static port_err_t s_show_grids_err = PORT_OK;
static port_err_t s_blank_err = PORT_OK;
static uint8_t s_brightness = 4u;
static uint8_t s_last_grids[TM1637_GRID_COUNT];

void fake_display_port_reset(void)
{
    s_op_count = 0u;
    s_power_on_err = PORT_OK;
    s_show_fill_err = PORT_OK;
    s_show_grids_err = PORT_OK;
    s_blank_err = PORT_OK;
    s_brightness = 4u;
    memset(s_last_grids, 0, sizeof(s_last_grids));
}

void fake_display_port_set_power_on_err(port_err_t err)
{
    s_power_on_err = err;
}

void fake_display_port_set_show_fill_err(port_err_t err)
{
    s_show_fill_err = err;
}

void fake_display_port_set_show_grids_err(port_err_t err)
{
    s_show_grids_err = err;
}

const fake_display_op_t *fake_display_port_ops(size_t *count)
{
    if (count != NULL) {
        *count = s_op_count;
    }
    return s_ops;
}

const fake_display_op_t *fake_display_port_last_grids(uint8_t grids[TM1637_GRID_COUNT])
{
    if (grids != NULL) {
        memcpy(grids, s_last_grids, sizeof(s_last_grids));
    }
    return s_ops;
}

uint8_t fake_display_port_brightness(void)
{
    return s_brightness;
}

static void fake_display_record(fake_display_op_kind_t kind)
{
    if (s_op_count >= FAKE_DISPLAY_MAX_OPS) {
        return;
    }

    s_ops[s_op_count].kind = kind;
    s_ops[s_op_count].segment_byte = 0u;
    memset(s_ops[s_op_count].grids, 0, sizeof(s_ops[s_op_count].grids));
    s_ops[s_op_count].brightness_level = s_brightness;
    s_op_count++;
}

static port_err_t fake_display_power_on(void)
{
    if (s_power_on_err != PORT_OK) {
        return s_power_on_err;
    }

    fake_display_record(FAKE_DISPLAY_OP_POWER_ON);
    return PORT_OK;
}

static port_err_t fake_display_power_off(void)
{
    fake_display_record(FAKE_DISPLAY_OP_POWER_OFF);
    return PORT_OK;
}

static port_err_t fake_display_show_fill(uint8_t segment_byte)
{
    if (s_show_fill_err != PORT_OK) {
        return s_show_fill_err;
    }

    if (s_op_count < FAKE_DISPLAY_MAX_OPS) {
        fake_display_record(FAKE_DISPLAY_OP_SHOW_FILL);
        s_ops[s_op_count - 1u].segment_byte = segment_byte;
    }
    return PORT_OK;
}

static port_err_t fake_display_show_grids(const uint8_t grids[TM1637_GRID_COUNT])
{
    if (s_show_grids_err != PORT_OK) {
        return s_show_grids_err;
    }

    if (grids != NULL) {
        memcpy(s_last_grids, grids, sizeof(s_last_grids));
    }

    if (s_op_count < FAKE_DISPLAY_MAX_OPS) {
        fake_display_record(FAKE_DISPLAY_OP_SHOW_GRIDS);
        if (grids != NULL) {
            memcpy(s_ops[s_op_count - 1u].grids, grids, sizeof(s_ops[s_op_count - 1u].grids));
        }
    }
    return PORT_OK;
}

static port_err_t fake_display_blank(void)
{
    if (s_blank_err != PORT_OK) {
        return s_blank_err;
    }

    fake_display_record(FAKE_DISPLAY_OP_BLANK);
    return PORT_OK;
}

static port_err_t fake_display_set_brightness(uint8_t level)
{
    if (level < 1u || level > 4u) {
        return PORT_ERR_INVALID_ARG;
    }

    s_brightness = level;
    if (s_op_count < FAKE_DISPLAY_MAX_OPS) {
        fake_display_record(FAKE_DISPLAY_OP_SET_BRIGHTNESS);
        s_ops[s_op_count - 1u].brightness_level = level;
    }
    return PORT_OK;
}

static const display_port_t s_fake_display = {
    .power_on = fake_display_power_on,
    .power_off = fake_display_power_off,
    .show_fill = fake_display_show_fill,
    .show_grids = fake_display_show_grids,
    .blank = fake_display_blank,
    .set_brightness = fake_display_set_brightness,
};

const display_port_t *fake_display_port_get(void)
{
    return &s_fake_display;
}
