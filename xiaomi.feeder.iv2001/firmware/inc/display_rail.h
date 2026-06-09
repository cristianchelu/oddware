/*
 * Display power rail policy — spec/30-processes/display-driver.md
 */

#ifndef DISPLAY_RAIL_H
#define DISPLAY_RAIL_H

#include <stdbool.h>
#include <stdint.h>

#include "gpio_expander_port.h"
#include "port_err.h"

#define DISPLAY_RAIL_SETTLE_MS  100u

typedef struct display_rail_ctx {
    const gpio_expander_port_t *expander;
    void (*delay_ms)(uint32_t ms);
    bool settled;
} display_rail_ctx_t;

void display_rail_ctx_init(display_rail_ctx_t *ctx,
                           const gpio_expander_port_t *expander,
                           void (*delay_ms)(uint32_t ms));

port_err_t display_rail_on(display_rail_ctx_t *ctx);
port_err_t display_rail_off(display_rail_ctx_t *ctx);
bool display_rail_is_settled(const display_rail_ctx_t *ctx);

#endif /* DISPLAY_RAIL_H */
