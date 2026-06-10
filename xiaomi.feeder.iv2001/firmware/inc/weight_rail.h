/*
 * CS1270 power rail (AW9523B P0.2) — spec/30-processes/weighing.md
 */

#ifndef WEIGHT_RAIL_H
#define WEIGHT_RAIL_H

#include <stdbool.h>
#include <stdint.h>

#include "gpio_expander_port.h"
#include "port_err.h"

#define WEIGHT_RAIL_SETTLE_MS  50u

typedef struct weight_rail_ctx {
    const gpio_expander_port_t *expander;
    void (*delay_ms)(uint32_t ms);
    bool settled;
} weight_rail_ctx_t;

void weight_rail_ctx_init(weight_rail_ctx_t *ctx,
                          const gpio_expander_port_t *expander,
                          void (*delay_ms)(uint32_t ms));

port_err_t weight_rail_on(weight_rail_ctx_t *ctx);
port_err_t weight_rail_off(weight_rail_ctx_t *ctx);
bool weight_rail_is_settled(const weight_rail_ctx_t *ctx);

#endif /* WEIGHT_RAIL_H */
