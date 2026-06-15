/*
 * Dispense supervisor — spec/30-processes/dispense-cycle.md
 */

#ifndef DISPENSE_H
#define DISPENSE_H

#include <stdbool.h>
#include <stdint.h>

#define DISPENSE_PORTIONS_MIN  1u
#define DISPENSE_PORTIONS_MAX  15u

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
    DISPENSE_OUTCOME_SUCCESS = 0,
    DISPENSE_OUTCOME_STUCK,
} dispense_outcome_t;

typedef struct {
    dispense_kind_t kind;
    uint16_t target;
} app_dispense_request_t;

dispense_submit_result_t dispense_submit_portions(uint8_t portions);
bool dispense_is_active(void);
void dispense_start_from_request(const app_dispense_request_t *req);
bool dispense_on_burst_done(void);
bool dispense_on_motor_fault(void);
void dispense_poll(void);
void dispense_test_reset(void);

#endif /* DISPENSE_H */
