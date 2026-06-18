/*
 * Application event loop — spec/30-processes/app-event-loop.md
 */

#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

#define APP_BOWL_GRAMS_BASELINE_FRESH_MS  2000u

typedef struct {
    bool valid;
    int32_t grams;
    uint32_t sample_age_ms;
} app_bowl_grams_snapshot_t;

bool app_bowl_grams_snapshot(uint32_t now_ms, app_bowl_grams_snapshot_t *out);
void app_bowl_grams_notify_read(int32_t grams, bool valid, uint32_t now_ms);

#endif /* APP_H */
