/*
 * Feeding schedule — spec/30-processes/scheduler-engine.md
 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCHEDULE_MAX_SLOTS 32u
#define SCHEDULE_G_MIN       5u
#define SCHEDULE_G_MAX       150u

typedef enum {
    SCHEDULE_STATE_PENDING = 0,
    SCHEDULE_STATE_TO_BE_SKIPPED,
    SCHEDULE_STATE_SKIPPED,
    SCHEDULE_STATE_DISPENSING,
    SCHEDULE_STATE_DISPENSED,
    SCHEDULE_STATE_FAILED,
} schedule_state_t;

typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t days;
    uint8_t g;
    bool enabled;
} schedule_slot_config_t;

typedef struct {
    schedule_state_t state;
    bool skip_today;
    int16_t g_actual;
    bool fired_today;
} schedule_slot_runtime_t;

typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t g;
    int32_t in_min;
} schedule_next_t;

typedef void (*schedule_changed_fn)(void);

typedef enum {
    SCHEDULE_FIRE_OK = 0,
    SCHEDULE_FIRE_BUSY,
    SCHEDULE_FIRE_REJECTED,
} schedule_fire_result_t;

typedef schedule_fire_result_t (*schedule_fire_fn)(uint8_t hour,
                                                   uint8_t min,
                                                   uint8_t g);

typedef enum {
    SCHEDULE_DISPENSE_OK = 0,
    SCHEDULE_DISPENSE_UNDERFILL,
    SCHEDULE_DISPENSE_FAILED,
} schedule_dispense_outcome_t;

typedef struct {
    uint8_t hour;
    uint8_t min;
    int32_t grams;
    schedule_dispense_outcome_t outcome;
} schedule_dispense_result_t;

void schedule_init(void);
void schedule_set_changed_fn(schedule_changed_fn fn);
void schedule_set_fire_fn(schedule_fire_fn fn);

size_t schedule_slot_count(void);
bool schedule_get_slot(size_t index,
                       schedule_slot_config_t *cfg,
                       schedule_slot_runtime_t *rt);
bool schedule_global_enabled(void);
bool schedule_today_enabled(void);

bool schedule_set_slot(const schedule_slot_config_t *cfg);
bool schedule_delete_slot(uint8_t hour, uint8_t min);
bool schedule_toggle_slot(uint8_t hour, uint8_t min);
bool schedule_skip_slot(uint8_t hour, uint8_t min, bool skip);
bool schedule_set_global_enabled(bool enabled);
bool schedule_set_today_enabled(bool enabled);

void schedule_poll(uint32_t now_ms);
void schedule_on_dispense_complete(const schedule_dispense_result_t *result);

bool schedule_active_slot(uint8_t *hour_out, uint8_t *min_out);

int schedule_format_state_json(char *buf, size_t len);
int schedule_format_next_json(char *buf, size_t len);
bool schedule_compute_next(schedule_next_t *out);

const char *schedule_state_wire(schedule_state_t state);

void schedule_test_reset(void);

#endif /* SCHEDULE_H */
