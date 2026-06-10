#include <stddef.h>

#include "fake_motor_index_port.h"

static bool s_led_on;
static bool s_beam_open;
static port_err_t s_read_err = PORT_OK;
static port_err_t s_led_err = PORT_OK;
static uint32_t s_set_led_calls;

static port_err_t fake_motor_index_set_led(bool on)
{
    if (s_led_err != PORT_OK) {
        return s_led_err;
    }

    s_led_on = on;
    s_set_led_calls++;
    return PORT_OK;
}

static port_err_t fake_motor_index_read_beam_open(bool *beam_open)
{
    if (beam_open == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_read_err != PORT_OK) {
        return s_read_err;
    }

    *beam_open = s_beam_open;
    return PORT_OK;
}

static const motor_index_port_t s_fake_motor_index = {
    .set_led = fake_motor_index_set_led,
    .read_beam_open = fake_motor_index_read_beam_open,
};

void fake_motor_index_port_reset(void)
{
    s_led_on = false;
    s_beam_open = false;
    s_read_err = PORT_OK;
    s_led_err = PORT_OK;
    s_set_led_calls = 0u;
}

void fake_motor_index_port_set_led(bool on)
{
    s_led_on = on;
}

bool fake_motor_index_port_get_led(void)
{
    return s_led_on;
}

void fake_motor_index_port_set_beam_open(bool beam_open)
{
    s_beam_open = beam_open;
}

void fake_motor_index_port_set_read_err(port_err_t err)
{
    s_read_err = err;
}

void fake_motor_index_port_set_led_err(port_err_t err)
{
    s_led_err = err;
}

uint32_t fake_motor_index_port_set_led_calls(void)
{
    return s_set_led_calls;
}

const motor_index_port_t *fake_motor_index_port_get(void)
{
    return &s_fake_motor_index;
}

const motor_index_port_t *motor_index_port_get(void)
{
    return fake_motor_index_port_get();
}
