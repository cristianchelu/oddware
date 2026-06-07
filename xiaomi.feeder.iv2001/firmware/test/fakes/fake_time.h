#ifndef FAKE_TIME_H
#define FAKE_TIME_H

#include <stdint.h>

#include "FreeRTOS.h"

void fake_time_reset(void);
void fake_time_advance_ms(uint32_t ms);
TickType_t fake_time_ticks(void);

#endif /* FAKE_TIME_H */
