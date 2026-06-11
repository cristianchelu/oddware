/*
 * Host test provider — fake motor port vs motor_ctrl integration stack.
 */

#include <stdbool.h>
#include <stddef.h>

#include "fake_motor_port.h"
#include "motor_ctrl.h"
#include "motor_port.h"

static bool s_use_integration;

static port_err_t motor_port_integration_timed_forward_ms(uint32_t duration_ms)
{
    return motor_ctrl_request_timed_forward_ms(duration_ms);
}

static port_err_t motor_port_integration_timed_reverse_ms(uint32_t duration_ms)
{
    return motor_ctrl_request_timed_reverse_ms(duration_ms);
}

static port_err_t motor_port_integration_burst(uint8_t pulse_target,
                                               uint16_t timeout_ms)
{
    return motor_ctrl_request_burst(pulse_target, timeout_ms);
}

static port_err_t motor_port_integration_park(uint8_t max_pulses)
{
    return motor_ctrl_request_park(max_pulses);
}

static port_err_t motor_port_integration_stop(void)
{
    return motor_ctrl_request_stop();
}

static bool motor_port_integration_is_active(void)
{
    return motor_ctrl_is_active();
}

static const motor_port_t s_integration_motor_port = {
    .request_timed_forward_ms = motor_port_integration_timed_forward_ms,
    .request_timed_reverse_ms = motor_port_integration_timed_reverse_ms,
    .request_burst = motor_port_integration_burst,
    .request_park = motor_port_integration_park,
    .stop = motor_port_integration_stop,
    .is_active = motor_port_integration_is_active,
};

void motor_port_host_use_integration(bool use_integration)
{
    s_use_integration = use_integration;
}

void motor_port_host_reset(void)
{
    s_use_integration = false;
}

const motor_port_t *motor_port_get(void)
{
    if (s_use_integration) {
        return &s_integration_motor_port;
    }

    return fake_motor_port_get();
}
