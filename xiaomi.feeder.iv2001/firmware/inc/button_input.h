/*
 * Button debounce and press detection — spec/30-processes/button-handling.md
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

#define BUTTON_INPUT_DEBOUNCE_MS  50u
#define BUTTON_INPUT_PRESS_QUEUE_DEPTH  4u

void button_input_init(const button_port_t *port);
void button_input_reset(void);
void button_input_notify_irq(uint32_t now_ms);
void button_input_poll(uint32_t now_ms);
bool button_input_pop_press(button_id_t *id);

#endif /* BUTTON_INPUT_H */
