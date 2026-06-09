/*
 * Display rail (AW9523B P0.5) — spec/30-processes/display-driver.md
 */

#include <stddef.h>

#include "board_gpio_iv2001.h"
#include "display_rail.h"

void display_rail_ctx_init(display_rail_ctx_t *ctx,
                           const gpio_expander_port_t *expander,
                           void (*delay_ms)(uint32_t ms))
{
    ctx->expander = expander;
    ctx->delay_ms = delay_ms;
    ctx->settled = false;
}

port_err_t display_rail_on(display_rail_ctx_t *ctx)
{
    port_err_t err;

    if (ctx == NULL || ctx->expander == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    ctx->settled = false;
    err = ctx->expander->set_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                 BOARD_GPIO_DISPLAY_RAIL_PIN,
                                 true);
    if (err != PORT_OK) {
        return err;
    }

    if (ctx->delay_ms != NULL) {
        ctx->delay_ms(DISPLAY_RAIL_SETTLE_MS);
    }

    ctx->settled = true;
    return PORT_OK;
}

port_err_t display_rail_off(display_rail_ctx_t *ctx)
{
    if (ctx == NULL || ctx->expander == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    ctx->settled = false;
    return ctx->expander->set_pin(BOARD_GPIO_DISPLAY_RAIL_PORT,
                                  BOARD_GPIO_DISPLAY_RAIL_PIN,
                                  false);
}

bool display_rail_is_settled(const display_rail_ctx_t *ctx)
{
    if (ctx == NULL) {
        return false;
    }

    return ctx->settled;
}
