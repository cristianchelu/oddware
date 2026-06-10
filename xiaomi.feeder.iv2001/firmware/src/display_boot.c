/*
 * Boot light test — spec/30-processes/display-presentation.md § Software layering
 */

#include <stddef.h>

#include "display_boot.h"
#include "display_port.h"

__attribute__((weak)) void display_boot_delay_ms(uint32_t ms)
{
    (void)ms;
}

port_err_t display_boot_run(void)
{
    const display_port_t *dp = display_port_get();
    port_err_t err;

    if (dp == NULL) {
        return PORT_ERR_IO;
    }

    display_boot_delay_ms(DISPLAY_BOOT_PRE_POWER_MS);

    err = dp->power_on();
    if (err != PORT_OK) {
        return err;
    }

    err = dp->show_fill(0xFFu);
    if (err != PORT_OK) {
        return err;
    }

    display_boot_delay_ms(DISPLAY_BOOT_LIGHT_TEST_MS);

    err = dp->blank();
    if (err != PORT_OK) {
        return err;
    }

    /* Rail off + I2C release before connsys_init / WFCI SPI on GPIO12–16. */
    return dp->power_off();
}
