/*
 * AW9523B bootstrap — spec/30-processes/display-driver.md § Boot self-test step 2
 */

#include <stddef.h>

#include "board_gpio_iv2001.h"
#include "gpio_expander_bootstrap.h"

port_err_t gpio_expander_bootstrap(const gpio_expander_port_t *exp)
{
    port_err_t err;

    if (exp == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = exp->reset();
    if (err != PORT_OK) {
        return err;
    }

    return exp->configure(BOARD_GPIO_BOOT_DIR_P0,
                          BOARD_GPIO_BOOT_DIR_P1,
                          BOARD_GPIO_BOOT_OUT_P0,
                          BOARD_GPIO_BOOT_OUT_P1);
}
