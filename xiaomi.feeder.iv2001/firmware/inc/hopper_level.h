/*
 * Hopper published level compositor — spec/30-processes/hopper-sensing.md § hopper_level
 */

#ifndef HOPPER_LEVEL_H
#define HOPPER_LEVEL_H

#include <stdbool.h>
#include <stdint.h>

#include "dispense.h"

typedef enum {
    HOPPER_LEVEL_STATE_NORMAL = 0,
    HOPPER_LEVEL_STATE_LOW,
    HOPPER_LEVEL_STATE_EMPTY,
} hopper_level_state_t;

typedef struct {
    hopper_level_state_t level;
    uint32_t at_ms;
} hopper_level_state_transition_t;

#define HOPPER_LEVEL_TRANSITION_QUEUE_DEPTH  4u

void hopper_level_init(void);
void hopper_level_reset(void);
void hopper_level_poll(void);
dispense_outcome_t hopper_level_on_dispense_finished(dispense_outcome_t outcome,
                                                     int32_t raw_delta,
                                                     bool measured,
                                                     uint32_t now_ms);
void hopper_level_notify_dispense_complete(void);
hopper_level_state_t hopper_level_get(void);
bool hopper_level_pop_transition(hopper_level_state_transition_t *tr);

#endif /* HOPPER_LEVEL_H */
