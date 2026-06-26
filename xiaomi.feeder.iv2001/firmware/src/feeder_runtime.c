/*
 * Cross-module runtime snapshot — spec/40-architecture/task-model.md § Runtime snapshot
 */

#include "feeder_runtime.h"

static volatile bool s_dispense_active;

void feeder_runtime_init(void)
{
    s_dispense_active = false;
}

void feeder_runtime_set_dispense_active(bool active)
{
    s_dispense_active = active;
}

bool feeder_runtime_dispense_active(void)
{
    return s_dispense_active;
}

void feeder_runtime_test_reset(void)
{
    feeder_runtime_init();
}
