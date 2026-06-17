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
void app_process_received_event(const app_event_t *ev);
void app_drain_queued_events(void);
bool app_step(void);
void app_test_reset(void);
void app_test_reset_bowl_presence_log(void);
void app_test_clear_btn_log(void);
bool app_test_take_btn_log(char *buf, size_t len);
void app_weight_notify_dispense_complete(void);

#endif /* APP_H */
