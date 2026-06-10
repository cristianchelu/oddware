/*
 * Application event loop — spec/30-processes/app-event-loop.md
 */

#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include <stddef.h>

#include "app_event.h"

void app_start(void);
void app_dispatch(const app_event_t *ev);
bool app_step(void);
void app_test_reset(void);
void app_test_clear_btn_log(void);
bool app_test_take_btn_log(char *buf, size_t len);

#endif /* APP_H */
