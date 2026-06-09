#include "fake_display_port.h"
#include "tm1637.h"

#define FAKE_DISPLAY_MAX_OPS 8u

static fake_display_op_t s_ops[FAKE_DISPLAY_MAX_OPS];
static size_t s_op_count;
static port_err_t s_power_on_err = PORT_OK;
static port_err_t s_show_fill_err = PORT_OK;
static port_err_t s_blank_err = PORT_OK;

void fake_display_port_reset(void)
{
    s_op_count = 0u;
    s_power_on_err = PORT_OK;
    s_show_fill_err = PORT_OK;
    s_blank_err = PORT_OK;
}

void fake_display_port_set_power_on_err(port_err_t err)
{
    s_power_on_err = err;
}

void fake_display_port_set_show_fill_err(port_err_t err)
{
    s_show_fill_err = err;
}

const fake_display_op_t *fake_display_port_ops(size_t *count)
{
    if (count != NULL) {
        *count = s_op_count;
    }
    return s_ops;
}

static void fake_display_record(fake_display_op_kind_t kind, uint8_t segment_byte)
{
    if (s_op_count >= FAKE_DISPLAY_MAX_OPS) {
        return;
    }

    s_ops[s_op_count].kind = kind;
    s_ops[s_op_count].segment_byte = segment_byte;
    s_op_count++;
}

static port_err_t fake_display_power_on(void)
{
    if (s_power_on_err != PORT_OK) {
        return s_power_on_err;
    }

    fake_display_record(FAKE_DISPLAY_OP_POWER_ON, 0u);
    return PORT_OK;
}

static port_err_t fake_display_power_off(void)
{
    fake_display_record(FAKE_DISPLAY_OP_POWER_OFF, 0u);
    return PORT_OK;
}

static port_err_t fake_display_show_fill(uint8_t segment_byte)
{
    if (s_show_fill_err != PORT_OK) {
        return s_show_fill_err;
    }

    fake_display_record(FAKE_DISPLAY_OP_SHOW_FILL, segment_byte);
    return PORT_OK;
}

static port_err_t fake_display_show_grids(const uint8_t grids[TM1637_GRID_COUNT])
{
    (void)grids;

    if (s_show_fill_err != PORT_OK) {
        return s_show_fill_err;
    }

    fake_display_record(FAKE_DISPLAY_OP_SHOW_FILL, 0u);
    return PORT_OK;
}

static port_err_t fake_display_blank(void)
{
    if (s_blank_err != PORT_OK) {
        return s_blank_err;
    }

    fake_display_record(FAKE_DISPLAY_OP_BLANK, 0u);
    return PORT_OK;
}

static const display_port_t s_fake_display = {
    .power_on = fake_display_power_on,
    .power_off = fake_display_power_off,
    .show_fill = fake_display_show_fill,
    .show_grids = fake_display_show_grids,
    .blank = fake_display_blank,
};

const display_port_t *fake_display_port_get(void)
{
    return &s_fake_display;
}
