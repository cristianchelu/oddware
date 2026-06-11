/* Tests: spec/30-processes/uart-console.md § motor commands */

#include "fake_motor_port.h"

static uint32_t s_run_calls;
static uint32_t s_last_duration_ms;
static uint32_t s_reverse_calls;
static uint32_t s_last_reverse_duration_ms;
static port_err_t s_run_err = PORT_OK;
static port_err_t s_reverse_err = PORT_OK;

static port_err_t fake_motor_run_forward_ms(uint32_t duration_ms)
{
    s_run_calls++;
    s_last_duration_ms = duration_ms;
    return s_run_err;
}

static port_err_t fake_motor_run_reverse_ms(uint32_t duration_ms)
{
    s_reverse_calls++;
    s_last_reverse_duration_ms = duration_ms;
    return s_reverse_err;
}

static const motor_port_t s_fake_motor = {
    .run_forward_ms = fake_motor_run_forward_ms,
    .run_reverse_ms = fake_motor_run_reverse_ms,
};

void fake_motor_port_reset(void)
{
    s_run_calls = 0u;
    s_last_duration_ms = 0u;
    s_reverse_calls = 0u;
    s_last_reverse_duration_ms = 0u;
    s_run_err = PORT_OK;
    s_reverse_err = PORT_OK;
}

uint32_t fake_motor_port_run_calls(void)
{
    return s_run_calls;
}

uint32_t fake_motor_port_last_duration_ms(void)
{
    return s_last_duration_ms;
}

void fake_motor_port_set_run_err(port_err_t err)
{
    s_run_err = err;
}

void fake_motor_port_set_reverse_err(port_err_t err)
{
    s_reverse_err = err;
}

uint32_t fake_motor_port_reverse_calls(void)
{
    return s_reverse_calls;
}

uint32_t fake_motor_port_last_reverse_duration_ms(void)
{
    return s_last_reverse_duration_ms;
}

const motor_port_t *fake_motor_port_get(void)
{
    return &s_fake_motor;
}

const motor_port_t *motor_port_get(void)
{
    return fake_motor_port_get();
}
