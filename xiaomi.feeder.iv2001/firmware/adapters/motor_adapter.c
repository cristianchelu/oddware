/*
 * Motor port adapter — spec/40-architecture/ports.md
 */

#include "motor_ctrl.h"
#include "motor_port.h"

static port_err_t motor_port_request_timed_forward_ms(uint32_t duration_ms)
{
    return motor_ctrl_request_timed_forward_ms(duration_ms);
}

static port_err_t motor_port_request_timed_reverse_ms(uint32_t duration_ms)
{
    return motor_ctrl_request_timed_reverse_ms(duration_ms);
}

static port_err_t motor_port_request_burst(uint8_t pulse_target, uint16_t timeout_ms)
{
    return motor_ctrl_request_burst(pulse_target, timeout_ms);
}

static port_err_t motor_port_request_park(uint8_t max_pulses)
{
    return motor_ctrl_request_park(max_pulses);
}

static port_err_t motor_port_stop(void)
{
    return motor_ctrl_request_stop();
}

static bool motor_port_is_active(void)
{
    return motor_ctrl_is_active();
}

static const motor_port_t s_motor_port = {
    .request_timed_forward_ms = motor_port_request_timed_forward_ms,
    .request_timed_reverse_ms = motor_port_request_timed_reverse_ms,
    .request_burst = motor_port_request_burst,
    .request_park = motor_port_request_park,
    .stop = motor_port_stop,
    .is_active = motor_port_is_active,
};

const motor_port_t *motor_port_get(void)
{
    return &s_motor_port;
}
