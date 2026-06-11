#include <stddef.h>

#include "fake_motor_index_port.h"

static bool s_led_on;
static bool s_session_active;
static bool s_beam_open;
static port_err_t s_read_err = PORT_OK;
static port_err_t s_led_err = PORT_OK;
static uint32_t s_set_led_calls;
static uint32_t s_poll_busy_remaining;

static port_err_t fake_motor_index_set_led(bool on)
{
    if (s_led_err != PORT_OK) {
        return s_led_err;
    }

    s_led_on = on;
    s_set_led_calls++;
    return PORT_OK;
}

static port_err_t fake_motor_index_read_detector(bool *beam_open)
{
    if (beam_open == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_read_err != PORT_OK) {
        return s_read_err;
    }

    *beam_open = s_led_on && s_beam_open;
    return PORT_OK;
}

static port_err_t fake_motor_index_sense(bool *beam_open)
{
    port_err_t err;
    port_err_t off_err;

    err = fake_motor_index_set_led(true);
    if (err != PORT_OK) {
        return err;
    }

    err = fake_motor_index_read_detector(beam_open);
    off_err = fake_motor_index_set_led(false);
    if (s_session_active) {
        (void)fake_motor_index_set_led(true);
    }

    if (err != PORT_OK) {
        return err;
    }

    return off_err;
}

static port_err_t fake_motor_index_session_begin(void)
{
    port_err_t err;

    if (s_session_active) {
        return PORT_OK;
    }

    err = fake_motor_index_set_led(true);
    if (err == PORT_OK) {
        s_session_active = true;
    }

    return err;
}

static port_err_t fake_motor_index_session_end(void)
{
    port_err_t err;

    if (!s_session_active) {
        return PORT_OK;
    }

    s_session_active = false;
    err = fake_motor_index_set_led(false);
    return err;
}

static port_err_t fake_motor_index_poll(bool *beam_open)
{
    if (!s_session_active) {
        return PORT_ERR_IO;
    }

    if (s_poll_busy_remaining > 0u) {
        s_poll_busy_remaining--;
        return PORT_ERR_BUSY;
    }

    return fake_motor_index_read_detector(beam_open);
}

static const motor_index_port_t s_fake_motor_index = {
    .sense = fake_motor_index_sense,
    .session_begin = fake_motor_index_session_begin,
    .session_end = fake_motor_index_session_end,
    .poll = fake_motor_index_poll,
};

void fake_motor_index_port_reset(void)
{
    s_led_on = false;
    s_session_active = false;
    s_beam_open = false;
    s_read_err = PORT_OK;
    s_led_err = PORT_OK;
    s_set_led_calls = 0u;
    s_poll_busy_remaining = 0u;
}

void fake_motor_index_port_set_poll_busy_remaining(uint32_t count)
{
    s_poll_busy_remaining = count;
}

void fake_motor_index_port_set_led(bool on)
{
    s_led_on = on;
}

bool fake_motor_index_port_get_led(void)
{
    return s_led_on;
}

bool fake_motor_index_port_session_active(void)
{
    return s_session_active;
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

