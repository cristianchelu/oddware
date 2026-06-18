/* Tests: spec/30-processes/uart-console.md § motor / dispense commands */

#include "fake_motor_port.h"

static uint32_t s_timed_fwd_calls;
static uint32_t s_timed_rev_calls;
static uint32_t s_last_timed_fwd_ms;
static uint32_t s_last_timed_rev_ms;
static uint32_t s_burst_calls;
static uint8_t s_last_pulse_target;
static uint16_t s_last_timeout_ms;
static uint32_t s_park_calls;
static uint8_t s_last_max_pulses;
static bool s_active;
static bool s_defer_burst_active;
static port_err_t s_timed_fwd_err = PORT_OK;
static port_err_t s_timed_rev_err = PORT_OK;
static port_err_t s_burst_err = PORT_OK;
static port_err_t s_park_err = PORT_OK;

static port_err_t fake_motor_request_timed_forward_ms(uint32_t duration_ms)
{
    s_timed_fwd_calls++;
    s_last_timed_fwd_ms = duration_ms;
    s_active = true;
    return s_timed_fwd_err;
}

static port_err_t fake_motor_request_timed_reverse_ms(uint32_t duration_ms)
{
    s_timed_rev_calls++;
    s_last_timed_rev_ms = duration_ms;
    s_active = true;
    return s_timed_rev_err;
}

static port_err_t fake_motor_request_burst(uint8_t pulse_target, uint16_t timeout_ms)
{
    s_burst_calls++;
    s_last_pulse_target = pulse_target;
    s_last_timeout_ms = timeout_ms;
    if (!s_defer_burst_active) {
        s_active = true;
    }
    return s_burst_err;
}

static port_err_t fake_motor_request_park(uint8_t max_pulses)
{
    s_park_calls++;
    s_last_max_pulses = max_pulses;
    s_active = true;
    return s_park_err;
}

static port_err_t fake_motor_stop(void)
{
    s_active = false;
    return PORT_OK;
}

static bool fake_motor_is_active(void)
{
    return s_active;
}

static const motor_port_t s_fake_motor = {
    .request_timed_forward_ms = fake_motor_request_timed_forward_ms,
    .request_timed_reverse_ms = fake_motor_request_timed_reverse_ms,
    .request_burst = fake_motor_request_burst,
    .request_park = fake_motor_request_park,
    .stop = fake_motor_stop,
    .is_active = fake_motor_is_active,
};

void fake_motor_port_reset(void)
{
    s_timed_fwd_calls = 0u;
    s_timed_rev_calls = 0u;
    s_last_timed_fwd_ms = 0u;
    s_last_timed_rev_ms = 0u;
    s_burst_calls = 0u;
    s_last_pulse_target = 0u;
    s_last_timeout_ms = 0u;
    s_park_calls = 0u;
    s_last_max_pulses = 0u;
    s_active = false;
    s_defer_burst_active = false;
    s_timed_fwd_err = PORT_OK;
    s_timed_rev_err = PORT_OK;
    s_burst_err = PORT_OK;
    s_park_err = PORT_OK;
}

uint32_t fake_motor_port_timed_fwd_calls(void)
{
    return s_timed_fwd_calls;
}

uint32_t fake_motor_port_last_timed_fwd_ms(void)
{
    return s_last_timed_fwd_ms;
}

void fake_motor_port_set_timed_fwd_err(port_err_t err)
{
    s_timed_fwd_err = err;
}

void fake_motor_port_set_timed_rev_err(port_err_t err)
{
    s_timed_rev_err = err;
}

uint32_t fake_motor_port_timed_rev_calls(void)
{
    return s_timed_rev_calls;
}

uint32_t fake_motor_port_last_timed_rev_ms(void)
{
    return s_last_timed_rev_ms;
}

uint32_t fake_motor_port_burst_calls(void)
{
    return s_burst_calls;
}

uint8_t fake_motor_port_last_pulse_target(void)
{
    return s_last_pulse_target;
}

uint16_t fake_motor_port_last_timeout_ms(void)
{
    return s_last_timeout_ms;
}

void fake_motor_port_set_burst_err(port_err_t err)
{
    s_burst_err = err;
}

void fake_motor_port_set_active(bool active)
{
    s_active = active;
}

void fake_motor_port_set_defer_burst_active(bool defer)
{
    s_defer_burst_active = defer;
}

void fake_motor_port_complete_burst(void)
{
    s_active = false;
}

uint32_t fake_motor_port_park_calls(void)
{
    return s_park_calls;
}

const motor_port_t *fake_motor_port_get(void)
{
    return &s_fake_motor;
}
