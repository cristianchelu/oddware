/*
 * Button debounce and transition detection — spec/30-processes/button-handling.md
 */

#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "button_port.h"

typedef enum {
    BUTTON_ID_POWER = 0,
    BUTTON_ID_RESET,
    BUTTON_ID_DISPENSE,
} button_id_t;

typedef enum {
    BUTTON_EDGE_DOWN = 0,
    BUTTON_EDGE_UP,
} button_edge_t;

typedef struct {
    button_id_t id;
    button_edge_t edge;
    uint32_t at_ms;
} button_transition_t;

#define BUTTON_INPUT_DEBOUNCE_MS  50u
#define BUTTON_INPUT_TRANSITION_QUEUE_DEPTH  8u

void button_input_init(const button_port_t *port);
void button_input_reset(void);
void button_input_notify_irq(uint32_t now_ms);
void button_input_poll(uint32_t now_ms);
bool button_input_pop_transition(button_transition_t *tr);

#endif /* BUTTON_INPUT_H */
