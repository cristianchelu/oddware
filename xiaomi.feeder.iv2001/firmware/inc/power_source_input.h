/*
 * Mains sense debounce — spec/30-processes/power-state-machine.md § Mains sense input
 */

#ifndef POWER_SOURCE_INPUT_H
#define POWER_SOURCE_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "power_source_port.h"

typedef enum {
    POWER_SOURCE_MAINS = 0,
    POWER_SOURCE_BATTERY,
} power_source_t;

typedef enum {
    POWER_SOURCE_EDGE_MAINS = 0,
    POWER_SOURCE_EDGE_BATTERY,
} power_source_edge_t;

typedef struct {
    power_source_edge_t edge;
    uint32_t at_ms;
} power_source_transition_t;

#define POWER_SOURCE_INPUT_DEBOUNCE_MS           50u
#define POWER_SOURCE_INPUT_TRANSITION_QUEUE_DEPTH  4u

void power_source_input_init(const power_source_port_t *port);
void power_source_input_reset(void);
void power_source_input_notify_irq(uint32_t now_ms);
void power_source_input_poll(uint32_t now_ms);
power_source_t power_source_input_get(void);
bool power_source_input_is_valid(void);
bool power_source_input_pop_transition(power_source_transition_t *tr);

#endif /* POWER_SOURCE_INPUT_H */
