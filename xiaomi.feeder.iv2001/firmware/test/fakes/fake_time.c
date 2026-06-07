#include "fake_time.h"

static TickType_t s_ticks;

void fake_time_reset(void)
{
    s_ticks = 0;
}

void fake_time_advance_ms(uint32_t ms)
{
    s_ticks += (TickType_t)ms;
}

TickType_t fake_time_ticks(void)
{
    return s_ticks;
}
