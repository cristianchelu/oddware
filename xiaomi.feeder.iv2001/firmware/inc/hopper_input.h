/*
 * Hopper level debounce — spec/30-processes/hopper-sensing.md
 */

#ifndef HOPPER_INPUT_H
#define HOPPER_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "hopper_ir_port.h"

typedef enum {
    HOPPER_LEVEL_NORMAL = 0,
    HOPPER_LEVEL_LOW,
} hopper_level_t;

typedef struct {
    hopper_level_t level;
    uint32_t at_ms;
} hopper_level_transition_t;

#define HOPPER_INPUT_DEBOUNCE_INTERVAL_MS    1000u
#define HOPPER_INPUT_LOW_STREAK_REQUIRED     6u
#define HOPPER_INPUT_NORMAL_STREAK_REQUIRED  3u
#define HOPPER_INPUT_BACKGROUND_INTERVAL_MS  60000u
#define HOPPER_INPUT_TRANSITION_QUEUE_DEPTH  4u

void hopper_input_init(const hopper_ir_port_t *port);
void hopper_input_reset(void);
void hopper_input_notify_dispense_complete(void);
void hopper_input_poll(uint32_t now_ms);
hopper_level_t hopper_input_get_level(void);
bool hopper_input_almost_empty(void);
bool hopper_input_pop_transition(hopper_level_transition_t *tr);

#endif /* HOPPER_INPUT_H */
