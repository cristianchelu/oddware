/*
 * CS1270 power rail (AW9523B P0.2) — spec/30-processes/weighing.md
 */

#include <stddef.h>

#include "board_gpio_iv2001.h"
#include "weight_rail.h"

void weight_rail_ctx_init(weight_rail_ctx_t *ctx,
                          const gpio_expander_port_t *expander,
                          void (*delay_ms)(uint32_t ms))
{
    ctx->expander = expander;
    ctx->delay_ms = delay_ms;
    ctx->settled = false;
}

port_err_t weight_rail_on(weight_rail_ctx_t *ctx)
{
    port_err_t err;

    if (ctx == NULL || ctx->expander == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    ctx->settled = false;
    err = ctx->expander->set_pin(BOARD_GPIO_CS1270_PWR_PORT,
                                 BOARD_GPIO_CS1270_PWR_PIN,
                                 true);
    if (err != PORT_OK) {
        return err;
    }

    if (ctx->delay_ms != NULL) {
        ctx->delay_ms(WEIGHT_RAIL_SETTLE_MS);
    }

    ctx->settled = true;
    return PORT_OK;
}

port_err_t weight_rail_off(weight_rail_ctx_t *ctx)
{
    if (ctx == NULL || ctx->expander == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    ctx->settled = false;
    return ctx->expander->set_pin(BOARD_GPIO_CS1270_PWR_PORT,
                                  BOARD_GPIO_CS1270_PWR_PIN,
                                  false);
}

bool weight_rail_is_settled(const weight_rail_ctx_t *ctx)
{
    if (ctx == NULL) {
        return false;
    }

    return ctx->settled;
}
