#include <stddef.h>

#include "fake_hopper_ir_port.h"

static bool s_beam_blocked;
static port_err_t s_sense_err = PORT_OK;
static uint32_t s_sense_count;

static port_err_t fake_hopper_ir_sense(bool *beam_blocked)
{
    if (beam_blocked == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_sense_err != PORT_OK) {
        return s_sense_err;
    }

    s_sense_count++;
    *beam_blocked = s_beam_blocked;
    return PORT_OK;
}

static const hopper_ir_port_t s_fake_hopper_ir = {
    .sense = fake_hopper_ir_sense,
};

void fake_hopper_ir_port_reset(void)
{
    s_beam_blocked = false;
    s_sense_err = PORT_OK;
    s_sense_count = 0u;
}

uint32_t fake_hopper_ir_port_sense_count(void)
{
    return s_sense_count;
}

void fake_hopper_ir_port_set_beam_blocked(bool beam_blocked)
{
    s_beam_blocked = beam_blocked;
}

void fake_hopper_ir_port_set_sense_err(port_err_t err)
{
    s_sense_err = err;
}

const hopper_ir_port_t *fake_hopper_ir_port_get(void)
{
    return &s_fake_hopper_ir;
}

const hopper_ir_port_t *hopper_ir_port_get(void)
{
    return fake_hopper_ir_port_get();
}
