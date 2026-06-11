/*
 * Host test provider — fake index port vs production adapter on fake GPIO expander.
 */

#include <stdbool.h>

#include "fake_motor_index_port.h"
#include "motor_index_port.h"
#include "motor_index_port_adapter.h"

static bool s_use_adapter;

void motor_index_port_host_use_adapter(bool use_adapter)
{
    s_use_adapter = use_adapter;
}

void motor_index_port_host_reset(void)
{
    s_use_adapter = false;
}

const motor_index_port_t *motor_index_port_get(void)
{
    if (s_use_adapter) {
        return motor_index_port_adapter_get();
    }

    return fake_motor_index_port_get();
}
