/*
 * Application event loop — spec/30-processes/app-event-loop.md
 */

#ifndef APP_H
#define APP_H

#include <stdbool.h>

#include "app_event.h"

void app_start(void);
void app_dispatch(const app_event_t *ev);
bool app_step(void);
void app_test_reset(void);

#endif /* APP_H */
