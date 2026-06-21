/*
 * Dispense supervisor — spec/30-processes/dispense-cycle.md
 */

#ifndef DISPENSE_H
#define DISPENSE_H

#include <stdbool.h>
#include <stdint.h>

#define DISPENSE_PORTIONS_MIN         1u
#define DISPENSE_PORTIONS_MAX         15u
#define DISPENSE_BASELINE_FRESH_MS    2000u
#define DISPENSE_SETTLE_MS            3000u
#define DISPENSE_GRAMS_PER_PORTION    10u

typedef enum {
    DISPENSE_KIND_PORTIONS = 0,
    DISPENSE_KIND_GRAMS,
} dispense_kind_t;

typedef enum {
    DISPENSE_SUBMIT_OK = 0,
    DISPENSE_SUBMIT_BUSY,
    DISPENSE_SUBMIT_INVALID,
    DISPENSE_SUBMIT_QUEUE_FULL,
} dispense_submit_result_t;

typedef enum {
    DISPENSE_SOURCE_MQTT = 0,
    DISPENSE_SOURCE_UART,
    DISPENSE_SOURCE_BUTTON,
    DISPENSE_SOURCE_SCHEDULE,
} dispense_source_t;

typedef enum {
    DISPENSE_MODE_OPEN_LOOP = 0,
    DISPENSE_MODE_COMPENSATED,
} dispense_mode_t;

typedef enum {
    DISPENSE_OUTCOME_SUCCESS = 0,
    DISPENSE_OUTCOME_STUCK,
    DISPENSE_OUTCOME_UNDERFILL,
    DISPENSE_OUTCOME_EMPTY_HOPPER,
    DISPENSE_OUTCOME_ABORTED,
} dispense_outcome_t;

typedef struct {
    dispense_kind_t kind;
    uint16_t target;
    dispense_source_t source;
} app_dispense_request_t;

typedef struct {
    int32_t grams;
    bool grams_estimated;
    uint16_t target_g;
    dispense_outcome_t outcome;
    dispense_source_t source;
    dispense_mode_t mode;
    uint8_t batch_count;
    uint8_t portions;
    bool has_slot;
    uint8_t slot_hour;
    uint8_t slot_min;
} dispense_completion_t;

dispense_submit_result_t dispense_submit_portions(uint8_t portions,
                                                  dispense_source_t source);
dispense_submit_result_t dispense_submit_grams(uint8_t grams,
                                               dispense_source_t source);
bool dispense_is_active(void);
void dispense_start_from_request(const app_dispense_request_t *req);
bool dispense_on_burst_done(void);
bool dispense_on_motor_fault(void);

/* Called on EVT_DISPLAY_TICK / EVT_TIMER_TICK with monotonic now_ms. */
void dispense_poll(uint32_t now_ms);

uint8_t dispense_test_zero_delta_streak(void);
void dispense_test_reset(void);

#endif /* DISPENSE_H */
